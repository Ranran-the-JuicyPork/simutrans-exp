/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include "simlinemgmt.h"
#include "simline.h"
#include "simconvoi.h"
#include "gui/simwin.h"
#include "simworld.h"
#include "simtypes.h"
#include "simintr.h"

#include "dataobj/schedule.h"
#include "dataobj/loadsave.h"

#include "gui/schedule_list.h"

#include "player/simplay.h"


simlinemgmt_t::~simlinemgmt_t()
{
	destroy_win((ptrdiff_t)this);
	// and delete all lines ...
	while (!all_managed_lines.empty()) {
		linehandle_t line = all_managed_lines.back();
		all_managed_lines.pop_back();
		delete line.get_rep(); // detaching handled by line itself
	}
}


void simlinemgmt_t::add_line(linehandle_t new_line)
{
	all_managed_lines.append(new_line);
}


void simlinemgmt_t::delete_line(linehandle_t line)
{
	if (line.is_bound()) {
		all_managed_lines.remove(line);
		//destroy line object
		delete line.get_rep();
	}
}

void simlinemgmt_t::deregister_line(linehandle_t line)
{
	if (line.is_bound())
	{
		all_managed_lines.remove(line);
	}
}


void simlinemgmt_t::update_line(linehandle_t line, bool do_not_renew_stops)
{
	// when a line is updated, all managed convoys must get the new schedule!
	for (auto& cnv : line->get_convoys())
	{
		cnv->set_update_line(line);
		if(line->get_schedule()->get_count() < 2)
		{
			// If a new schedule is incomplete, convoys will
			// be blocking places unless sent to the depot.
			cnv->emergency_go_to_depot();
			// Note that this may destroy a convoi in extreme cases (no depot).
		}
		if(  cnv.is_bound() && cnv->in_depot()  )
		{
			cnv->check_pending_updates(); // apply new schedule immediately for convoys in depot
		}
	}
	if (!do_not_renew_stops)
	{
		// finally de/register all stops
		line->renew_stops();
		if (line->count_convoys() > 0)
		{
			world()->set_schedule_counter();
		}
	}
}


void simlinemgmt_t::rdwr(loadsave_t *file, player_t *player)
{
	xml_tag_t l( file, "simlinemgmt_t" );
	DBG_MESSAGE("simlinemgmt_t::rdwr", "player=%s",player->get_name());
	if(file->is_saving()) {
		// on old versions
		if(  file->is_version_less(101, 0)  ) {
			file->wr_obj_id("Linemanagement");
		}

		// ensure that lines are saved in the same order on all clients
		sort_lines();

		uint32 count = all_managed_lines.get_count();
		file->rdwr_long(count);
		for (auto const i : all_managed_lines)
		{
			simline_t::linetype lt = i->get_linetype();
			file->rdwr_enum(lt);
			i->rdwr(file);
		}
	}
	else {
		// on old versions
		if(  file->is_version_less(101, 0)  ) {
			char buf[80];
			file->rd_obj_id(buf, 79);
			all_managed_lines.clear();
			if(strcmp(buf, "Linemanagement")!=0) {
				dbg->fatal( "simlinemgmt_t::rdwr", "Broken Savegame" );
			}
		}
		sint32 totalLines = 0;
		file->rdwr_long(totalLines);
DBG_MESSAGE("simlinemgmt_t::rdwr()","number of lines=%i",totalLines);

		simline_t *unbound_line = NULL;

		for (int i = 0; i<totalLines; i++) {
			simline_t::linetype lt=simline_t::line;
			file->rdwr_enum(lt);
		DBG_MESSAGE("simlinemgmt_t::rdwr","linetype=%s",simline_t::get_linetype_name(lt));

			if(lt < simline_t::truckline  ||  lt > simline_t::narrowgaugeline) {
					dbg->fatal( "simlinemgmt_t::rdwr()", "Cannot create default line!" );
			}
			simline_t *line = new simline_t(player, lt, file);
			if (!line->get_handle().is_bound()) {
				// line id was saved as zero ...
				if (unbound_line) {
					dbg->fatal( "simlinemgmt_t::rdwr()", "More than one line with unbound id read" );
				}
				else {
					unbound_line = line;
				}
			}
			else {
				add_line( line->get_handle() );
			}
		}

		if(  unbound_line  ) {
			// linehandle will be corrected in simline_t::finish_rd
			line_with_id_zero = linehandle_t(unbound_line,true);
			add_line( line_with_id_zero );
		}
	}
}


static bool compare_lines(linehandle_t const a, linehandle_t const b)
{
	int diff = 0;
	char const* const na = a->get_name();
	char const* const nb = b->get_name();
	if (na[0] == '(' && nb[0] == '(') {
		diff = atoi(na + 1) - atoi(nb + 1);
	}
	if(  diff==0  ) {
		diff = strcmp(na, nb);
	}
	if(diff==0) {
		diff = a.get_id() - b.get_id();
	}
	return diff < 0;
}


void simlinemgmt_t::sort_lines()
{
	std::sort(all_managed_lines.begin(), all_managed_lines.end(), compare_lines);
}


void simlinemgmt_t::finish_rd()
{
	for (auto const i : all_managed_lines)
	{
		i->finish_rd();
	}
	sort_lines();
}


void simlinemgmt_t::rotate90( sint16 y_size )
{
	for (auto const i : all_managed_lines)
	{
		if (schedule_t* const schedule = i->get_schedule()) {
			schedule->rotate90( y_size );
		}
	}
}


void simlinemgmt_t::new_month()
{
	for (auto const i : all_managed_lines)
	{
		i->new_month();
	}
}


linehandle_t simlinemgmt_t::create_line(int ltype, player_t * player)
{
	if(ltype < simline_t::truckline  ||  ltype > simline_t::narrowgaugeline) {
			dbg->fatal( "simlinemgmt_t::create_line()", "Cannot create default line!" );
	}

	simline_t * line = new simline_t(player, (simline_t::linetype)ltype);

	add_line( line->get_handle() );
	sort_lines();
	return line->get_handle();
}


linehandle_t simlinemgmt_t::create_line(int ltype, player_t * player, schedule_t * schedule)
{
	linehandle_t line = create_line( ltype, player );
	if(schedule) {
		line->get_schedule()->copy_from(schedule);
	}
	return line;
}


void simlinemgmt_t::get_lines(int type, vector_tpl<linehandle_t>* lines, uint8 freight_type_bits, bool show_empty_line) const
{
	lines->clear();
	for (auto const line : all_managed_lines)
	{
		if (type == simline_t::line || line->get_linetype() == simline_t::line || line->get_linetype() == type) {
			if (!show_empty_line && !line->get_convoys().get_count()) {
				continue;
			}
			if (freight_type_bits) {
				if (freight_type_bits & (1 << simline_t::all_pas) && line->get_goods_catg_index().is_contained(goods_manager_t::INDEX_PAS)) {
					lines->append(line);
					continue;
				}
				if (freight_type_bits & (1 << simline_t::all_mail) && line->get_goods_catg_index().is_contained(goods_manager_t::INDEX_MAIL)) {
					lines->append(line);
					continue;
				}
				if (freight_type_bits & (1 << simline_t::all_freight)) {
					for (uint8 catg_index = goods_manager_t::INDEX_NONE + 1; catg_index < goods_manager_t::get_max_catg_index(); catg_index++)
					{
						if (line->get_goods_catg_index().is_contained(catg_index)) {
							lines->append(line);
							break;
						}
					}
				}
			}
			else {
				lines->append(line);
			}
		}
	}
}


void simlinemgmt_t::show_lineinfo(player_t *player, linehandle_t line)
{
	gui_frame_t *schedule_list_gui = win_get_magic( magic_line_management_t + player->get_player_nr() );
	if(  schedule_list_gui  ) {
		top_win( schedule_list_gui );
	}
	else {
		schedule_list_gui = new schedule_list_gui_t(player);
		create_win( schedule_list_gui, w_info, magic_line_management_t+player->get_player_nr() );
	}
	dynamic_cast<schedule_list_gui_t *>(schedule_list_gui)->show_lineinfo(line);
}
