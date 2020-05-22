/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "factorylist_frame_t.h"

#include "../dataobj/translator.h"

/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool factorylist_frame_t::sortreverse = false;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Station number
 *         1 = Station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author Markus Weber
 */
factorylist::sort_mode_t factorylist_frame_t::sortby = factorylist::by_name;
static uint8 default_sortmode = 0;

// filter by within current player's network
bool factorylist_frame_t::filter_own_network = false;
uint8 factorylist_frame_t::filter_goods_catg = goods_manager_t::INDEX_NONE;

const char *factorylist_frame_t::sort_text[factorylist::SORT_MODES] = {
	"Fabrikname",
	"Input",
	"Output",
	"Produktion",
	"Rating",
	"Power",
	"Sector",
	"Staffing"
};

factorylist_frame_t::factorylist_frame_t() :
	gui_frame_t( translator::translate("fl_title") ),
	sort_label(translator::translate("hl_txt_sort")),
	stats(sortby,sortreverse, filter_own_network, filter_goods_catg),
	scrolly(&stats)
{
	sort_label.set_pos(scr_coord(BUTTON1_X, 2));
	add_component(&sort_label);

	sortedby.set_pos(scr_coord(BUTTON1_X, 14));
	sortedby.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_BUTTON_HEIGHT));
	sortedby.set_max_size(scr_size(D_BUTTON_WIDTH*1.5, LINESPACE * 4));

	for (int i = 0; i < factorylist::SORT_MODES; i++) {
		sortedby.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(sort_text[i]), SYSCOL_TEXT));
	}
	sortedby.set_selection(default_sortmode);

	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, "", scr_coord(BUTTON1_X + D_BUTTON_WIDTH*1.5, 14), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_component(&sorteddir);

	freight_type_c.set_pos(scr_coord(BUTTON2_X + +D_BUTTON_WIDTH * 1.5 + D_H_SPACE, 14));
	freight_type_c.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_BUTTON_HEIGHT));
	freight_type_c.set_max_size(scr_size(D_BUTTON_WIDTH*1.5, LINESPACE * 4));
	{
		int count = 0;
		viewable_freight_types.append(NULL);
		freight_type_c.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("All"), SYSCOL_TEXT));
		for (int i = 0; i < goods_manager_t::get_max_catg_index(); i++) {
			const goods_desc_t *freight_type = goods_manager_t::get_info_catg(i);
			const int index = freight_type->get_catg_index();
			if (index == goods_manager_t::INDEX_NONE || freight_type->get_catg() == 0) {
				continue;
			}
			freight_type_c.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(freight_type->get_catg_name()), SYSCOL_TEXT));
			viewable_freight_types.append(freight_type);
		}
		for (int i = 0; i < goods_manager_t::get_count(); i++) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if (ware->get_catg() == 0 && ware->get_index() > 2) {
				// Special freight: Each good is special
				viewable_freight_types.append(ware);
				freight_type_c.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(ware->get_name()), SYSCOL_TEXT));
			}
		}
	}
	freight_type_c.set_selection(filter_goods_catg = goods_manager_t::INDEX_NONE ? 0 : filter_goods_catg);
	set_filter_goods_catg(filter_goods_catg);

	freight_type_c.add_listener(this);
	add_component(&freight_type_c);

	filter_within_network.init(button_t::square_state, "Within own network", scr_coord(BUTTON2_X + +D_BUTTON_WIDTH*1.5+ D_H_SPACE, 2));
	filter_within_network.set_tooltip("Show only connected to own transport network");
	filter_within_network.add_listener(this);
	filter_within_network.pressed = filter_own_network;
	add_component(&filter_within_network);

	// name buttons
	sorteddir.set_text(get_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");

	scrolly.set_pos(scr_coord(0, 14+D_BUTTON_HEIGHT+2));
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly);

	display_list();

	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+18*(LINESPACE+1)+14+D_BUTTON_HEIGHT+2+1));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4*(LINESPACE+1)+14+D_BUTTON_HEIGHT+2+1));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber/Volker Meyer
 */
bool factorylist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &sortedby) {
		int tmp = sortedby.get_selection();
		if (tmp >= 0 && tmp < sortedby.count_elements())
		{
			sortedby.set_selection(tmp);
			set_sortierung((factorylist::sort_mode_t)tmp);
		}
		else {
			sortedby.set_selection(0);
			set_sortierung(factorylist::by_name);
		}
		default_sortmode = (uint8)tmp;
		display_list();
	}
	else if(comp == &sorteddir) {
		set_reverse(!get_reverse());
		sorteddir.set_text(get_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		display_list();
	}
	else if (comp == &filter_within_network) {
		filter_own_network = !filter_own_network;
		filter_within_network.pressed = filter_own_network;
		display_list();
	}
	else if (comp == &freight_type_c) {
		if (freight_type_c.get_selection() > 0) {
			filter_goods_catg = viewable_freight_types[freight_type_c.get_selection()]->get_catg_index();
		}
		else if (freight_type_c.get_selection() == 0) {
			filter_goods_catg = goods_manager_t::INDEX_NONE;
		}
		display_list();
	}
	return true;
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void factorylist_frame_t::resize(const scr_coord delta)
{
	gui_frame_t::resize(delta);
	// window size - titlebar - offset (header)
	scr_size size = get_windowsize()-scr_size(0,D_TITLEBAR_HEIGHT+14+D_BUTTON_HEIGHT+2+1);
	scrolly.set_size(size);
	sortedby.set_max_size(scr_size(D_BUTTON_WIDTH*1.5, scrolly.get_size().h));
	freight_type_c.set_max_size(scr_size(D_BUTTON_WIDTH*1.5, scrolly.get_size().h));
}


void factorylist_frame_t::display_list()
{
	sorteddir.set_text(get_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	stats.sort(sortby, get_reverse(), get_filter_own_network(), filter_goods_catg);
	stats.recalc_size();
}
