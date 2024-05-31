/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_factory_storage_info.h"
#include "gui_image.h"
#include "gui_halt_indicator.h"

#include "../../simcolor.h"
#include "../../simworld.h"

#include "../../dataobj/environment.h"
#include "../../dataobj/translator.h"
#include "../../utils/cbuffer_t.h"

#include "../../display/viewport.h"
#include "../../utils/simstring.h"

#include "../../player/simplay.h"

#include "../map_frame.h"
#include "../simwin.h"

#define STORAGE_INDICATOR_WIDTH (50)


gui_operation_status_t::gui_operation_status_t(PIXVAL c)
{
	color = c;
	tooltip = NULL;
	gui_component_t::set_size(scr_size(D_FIXED_SYMBOL_WIDTH, LINESPACE));
}

void gui_operation_status_t::draw(scr_coord offset)
{
	offset += pos;
	const uint8 yoff = D_GET_CENTER_ALIGN_OFFSET(GOODS_COLOR_BOX_HEIGHT, size.h);
	switch (status)
	{
		case operation_stop:
			display_fillbox_wh_clip_rgb(offset.x+2, offset.y+yoff, GOODS_COLOR_BOX_HEIGHT-2, GOODS_COLOR_BOX_HEIGHT-2, color, false);
			break;
		case operation_pause:
			display_fillbox_wh_clip_rgb(offset.x+2,   offset.y+yoff, GOODS_COLOR_BOX_HEIGHT/3, GOODS_COLOR_BOX_HEIGHT-2, color, false);
			display_fillbox_wh_clip_rgb(offset.x+2+ GOODS_COLOR_BOX_HEIGHT/3*2, offset.y+yoff, GOODS_COLOR_BOX_HEIGHT/3, GOODS_COLOR_BOX_HEIGHT-2, color, false);
			break;
		case operation_normal:
			display_right_pointer_rgb(offset.x, offset.y+yoff, GOODS_COLOR_BOX_HEIGHT, color, false);
			break;
		case operation_ok:
			display_filled_circle_rgb(offset.x+(GOODS_COLOR_BOX_HEIGHT>>1)+1, offset.y+yoff+(GOODS_COLOR_BOX_HEIGHT>>1), GOODS_COLOR_BOX_HEIGHT>>1, color);
			break;
		case operation_caution:
			if (skinverwaltung_t::alerts) {
				display_color_img(skinverwaltung_t::alerts->get_image_id(2), offset.x, offset.y + FIXED_SYMBOL_YOFF, 0, false, false);
			}
			else {
				display_proportional_clip_rgb(offset.x, offset.y, "!", ALIGN_LEFT, COL_CAUTION, false);
			}
			break;
		case operation_warning:
			if (skinverwaltung_t::alerts) {
				display_color_img(skinverwaltung_t::alerts->get_image_id(4), offset.x, offset.y + FIXED_SYMBOL_YOFF, 0, false, false);
			}
			else {
				display_proportional_clip_rgb(offset.x, offset.y, "X", ALIGN_LEFT, COL_CAUTION, false);
			}
			break;
		case operation_invalid:
		default:
			break;
	}
}


gui_factory_operation_status_t::gui_factory_operation_status_t(fabrik_t *factory) : gui_operation_status_t{}
{
	fab = factory;
}

void gui_factory_operation_status_t::draw(scr_coord offset)
{
	const uint8 avtivity_score = min(6, (uint8)((fab->get_stat(0, FAB_PRODUCTION) + 1999) / 2000));
	const PIXVAL producing_status_color = color_idx_to_rgb(severity_color[avtivity_score]);
	set_color((!avtivity_score && fab->is_staff_shortage()) ? SYSCOL_STAFF_SHORTAGE : producing_status_color);
	if (avtivity_score) {
		set_status(gui_operation_status_t::operation_ok);
	}
	else if (fab->get_nearby_freight_halts().empty()) {
		set_status(gui_operation_status_t::operation_invalid);
	}
	else if (!fab->get_output().empty() && fab->get_total_out() == fab->get_total_output_capacity()) {
		// The output inventory is full, so it is suspending production
		set_status(gui_operation_status_t::operation_pause);
	}
	else {
		set_status(gui_operation_status_t::operation_stop);
	}

	gui_operation_status_t::draw(offset);
}

gui_factory_storage_bar_t::gui_factory_storage_bar_t(const ware_production_t *ware, uint32 factor, bool is_input)
{
	this->ware = ware;
	this->is_input_item = is_input;
	this->factor = factor;
	set_size(get_min_size());
}

void gui_factory_storage_bar_t::draw(scr_coord offset)
{
	offset += pos;

	const scr_coord_val color_bar_w = size.w-2;

	display_ddd_box_clip_rgb(offset.x, offset.y, size.w, size.h, color_idx_to_rgb(MN_GREY0), color_idx_to_rgb(MN_GREY4));
	display_fillbox_wh_clip_rgb(offset.x + 1, offset.y+1, color_bar_w, size.h-2, color_idx_to_rgb(MN_GREY2), false);

	uint32 substantial_intransit = 0; // yellowed bar

	if ( ware->get_typ()->is_available() ) {
		const uint32 storage_capacity = (uint32)ware->get_capacity(factor);
		if (storage_capacity == 0) { return; }
		const uint32 stock_quantity = min((uint32)ware->get_storage(), storage_capacity);

		const PIXVAL goods_color = ware->get_typ()->get_color();
		display_cylinderbar_wh_clip_rgb(offset.x+1, offset.y+1, color_bar_w*stock_quantity/storage_capacity, size.h-2, goods_color, true);

		// input has in_transit
		if (is_input_item) {
			substantial_intransit = min((uint32)ware->get_in_transit(), storage_capacity-stock_quantity);
			if (substantial_intransit) {
				// in transit for input storage
				display_fillbox_wh_clip_rgb(offset.x+1+color_bar_w* stock_quantity/storage_capacity, offset.y+1, color_bar_w*substantial_intransit/storage_capacity, size.h-2, COL_IN_TRANSIT, false);
			}
		}
	}
}

gui_factory_storage_label_t::gui_factory_storage_label_t(const ware_production_t *ware, uint32 capacity)
{
	this->ware = ware;
	this->capacity = capacity;
}

void gui_factory_storage_label_t::draw(scr_coord offset)
{
	if (old_storage != ware->get_storage()) {
		old_storage = ware->get_storage();
		if (ware->get_typ()->is_available()) {
			buf().printf("%3u/%u%s", old_storage, capacity, translator::translate(ware->get_typ()->get_mass()));
			set_color(SYSCOL_TEXT);
		}
		else {
			buf().printf(" %s", translator::translate("n/a"));
			set_color(SYSCOL_TEXT_WEAK);
		}
		update();
	}
	gui_label_buf_t::draw(offset);
}

gui_factory_intransit_label_t::gui_factory_intransit_label_t(const ware_production_t *ware, uint32 pfactor)
{
	this->ware = ware;
	factor = pfactor;
}

void gui_factory_intransit_label_t::draw(scr_coord offset)
{
	if (old_intransit != ware->get_in_transit() || old_max != ware->max_transit) {
		old_intransit = ware->get_in_transit();
		old_max = ware->max_transit;
		buf().printf("%3i/%i", old_intransit, convert_goods(ware->max_transit*factor));
		update();
	}
	gui_label_buf_t::draw(offset);
}


gui_factory_monthly_prod_label_t::gui_factory_monthly_prod_label_t(fabrik_t *factory, const goods_desc_t *goods, uint32 factor, bool is_input)
{
	fab = factory;
	this->goods = goods;
	is_input_item = is_input;
	pfactor = factor;
}

void gui_factory_monthly_prod_label_t::draw(scr_coord offset)
{
	const uint32 monthly_prod = (uint32)(fab->get_current_production()*pfactor * 10 >> DEFAULT_PRODUCTION_FACTOR_BITS);
	if (is_input_item) {
		if (monthly_prod < 100) {
			buf().printf(translator::translate("production %.1f%s/month"), (float)monthly_prod/10.0, translator::translate(goods->get_mass()));
		}
		else {
			buf().printf(translator::translate("production %u%s/month"), monthly_prod/10, translator::translate(goods->get_mass()));
		}
	}
	else {
		if (monthly_prod < 100) {
			buf().printf(translator::translate("consumption %.1f%s/month"), (float)monthly_prod/10.0, translator::translate(goods->get_mass()));
		}
		else {
			buf().printf(translator::translate("consumption %u%s/month"), monthly_prod/10, translator::translate(goods->get_mass()));
		}
	}
	update();
	gui_label_buf_t::draw(offset);
}


gui_factory_product_item_t::gui_factory_product_item_t(fabrik_t *factory, const ware_production_t *ware, bool is_input_item) :
	lb_leadtime(""), lb_alert("")
{
	this->fab = factory;
	this->ware = ware;
	this->is_input_item = is_input_item;
	lb_leadtime.set_image(skinverwaltung_t::travel_time ? skinverwaltung_t::travel_time->get_image_id(0) : IMG_EMPTY);
	lb_leadtime.set_tooltip(translator::translate("symbol_help_txt_lead_time"));
	lb_alert.set_visible(false);

	if (fab) {
		set_table_layout(7,0);
		init_table();
	}
}

void gui_factory_product_item_t::init_table()
{
	goods_desc_t const* const desc = ware->get_typ();

	// material arrival status for input (vs supplier)
	if (is_input_item) {
		add_component(&operation_status);
	}
	new_component<gui_image_t>(desc->get_catg_symbol(), 0, ALIGN_CENTER_V, true);
	new_component<gui_colorbox_t>(desc->get_color())->set_size(GOODS_COLOR_BOX_SIZE);
	new_component<gui_label_t>(desc->get_name(), ware->get_typ()->is_available() ? SYSCOL_TEXT : SYSCOL_TEXT_WEAK);
	// material delivered status for output (vs consumer)
	if (!is_input_item) {
		add_component(&operation_status);
	}
	new_component<gui_margin_t>(LINESPACE);
	add_component(&lb_leadtime);
	lb_leadtime.set_visible(is_input_item);
	add_component(&lb_alert);
}

void gui_factory_product_item_t::draw(scr_coord offset)
{
	// update operation status
	if (is_input_item) {
		// lead time
		uint32 lead_time = fab->get_lead_time(ware->get_typ());
		if (lead_time == UINT32_MAX_VALUE) {
			lb_leadtime.buf().append("--:--:--");
		}
		else {
			char lead_time_as_clock[32];
			world()->sprintf_time_tenths(lead_time_as_clock, 32, lead_time);
			lb_leadtime.buf().append(lead_time_as_clock);
			//col_val = is_connected_to_own_network ? SYSCOL_TEXT : SYSCOL_TEXT_INACTIVE;
		}
		lb_leadtime.update();

		sint32 goods_needed = fab->goods_needed(ware->get_typ());

		// operation status, reciept/intansit or not
		uint8 reciept_score = 0;
		if (ware->get_stat(0, FAB_GOODS_RECEIVED)) {
			// This factory is receiving materials this month.
			reciept_score += 80;
		}
		if (ware->get_stat(1, FAB_GOODS_RECEIVED)) {
			// This factory hasn't recieved this month yet, but it did last month.
			reciept_score = min(100, reciept_score + 50);
		}
		if (ware->get_stat(0, FAB_GOODS_TRANSIT)) {
			reciept_score = min(100, reciept_score + 20);
		}
		operation_status.set_color(color_idx_to_rgb(severity_color[(reciept_score+19)/20]));
		if (!reciept_score) {
			if (goods_needed <= 0) {
				// No shipments demanded: sufficient product is in stock
				operation_status.set_status(gui_operation_status_t::operation_pause);
				lb_alert.set_image(skinverwaltung_t::pax_evaluation_icons ? skinverwaltung_t::pax_evaluation_icons->get_image_id(4) : IMG_EMPTY);
				lb_alert.buf().append(translator::translate("Shipment is suspended"));
				lb_alert.set_tooltip(translator::translate("Shipment has been suspended due to consumption demand"));
				lb_alert.update();
				lb_alert.set_visible(true);
			}
			else {
				operation_status.set_color(COL_INACTIVE);
				operation_status.set_status(gui_operation_status_t::operation_stop);
				lb_alert.set_visible(false);
			}
		}
		else {
			lb_alert.set_visible(false);
			if (goods_needed <= 0) {
				lb_alert.set_image(skinverwaltung_t::alerts ? skinverwaltung_t::alerts->get_image_id(3) : IMG_EMPTY);
				lb_alert.buf().append(translator::translate("Shipment is suspended"));
				lb_alert.set_tooltip(translator::translate("Suspension of new orders due to sufficient supply"));
				lb_alert.update();
				lb_alert.set_visible(true);
			}
			operation_status.set_status(gui_operation_status_t::operation_normal);
		}
	}
	else {
		uint8 shipping_score = 0;
		if (ware->get_stat(0, FAB_GOODS_DELIVERED)) {
			// This factory is shipping this month.
			shipping_score += 80;
		}
		if (ware->get_stat(1, FAB_GOODS_DELIVERED)) {
			// This factory hasn't shipped this month yet, but it did last month.
			shipping_score = min(100, shipping_score + 50);
		}
		if (!shipping_score) {
			if (!ware->get_stat(0, FAB_GOODS_STORAGE)) {
				operation_status.set_status(gui_operation_status_t::operation_stop);
				operation_status.set_color(fab->is_staff_shortage() ? SYSCOL_STAFF_SHORTAGE : SYSCOL_TEXT_WEAK);
			}
			else {
				// Stopped due to demand/connection issue
				operation_status.set_status(gui_operation_status_t::operation_pause);
				operation_status.set_color(color_idx_to_rgb(COL_RED+1));
			}
		}
		else {
			operation_status.set_color(color_idx_to_rgb(severity_color[(shipping_score+19)/20]));
			operation_status.set_status(gui_operation_status_t::operation_normal);
		}
	}

	gui_aligned_container_t::draw(offset);
}

bool gui_factory_product_item_t::infowin_event(const event_t *ev)
{
	if (IS_LEFTRELEASE(ev)) {
		ware->get_typ()->show_info();
	}
	return gui_aligned_container_t::infowin_event(ev);
}


// component for factory storage display
gui_factory_storage_info_t::gui_factory_storage_info_t(fabrik_t* factory, bool is_output) :
	lb_caption(is_output ? translator::translate("Produktion") : translator::translate("Verbrauch") ),
	lb_alert( is_output ? translator::translate("Storage full") : translator::translate("No material"))
{
	this->fab = factory;
	this->is_output = is_output;
	set_table_layout(1,0);
	if (fab) {
		init_table();
	}
}

void gui_factory_storage_info_t::init_table()
{
	remove_all();
	const uint32 rows = is_output ? fab->get_output().get_count() : fab->get_input().get_count();
	// input storage info (Consumption)
	if (rows) {
		if (skinverwaltung_t::input_output) {
			lb_caption.set_image( skinverwaltung_t::input_output->get_image_id(is_output ? 1:0) );
		}
		if (skinverwaltung_t::alerts) {
			lb_alert.set_image(skinverwaltung_t::alerts->get_image_id(2));
		}
		add_table(2,1);
		{
			add_component(&lb_caption);
			lb_alert.set_visible(false);
			add_component(&lb_alert);
		}
		end_table();

		add_table(7,rows)->set_spacing(scr_size(D_H_SPACE,0));
		{
			int i = 0;
			FORX(array_tpl<ware_production_t>, const& goods, is_output ? fab->get_output() : fab->get_input(), i++) {
				if(!is_output && !fab->get_desc()->get_supplier(i) ) {
					continue;
				}
				const bool is_available = goods.get_typ()->is_available();
				const sint64 pfactor = is_output ? (sint64)fab->get_desc()->get_product(i)->get_factor()
					: (fab->get_desc()->get_supplier(i) ? (sint64)fab->get_desc()->get_supplier(i)->get_consumption() : 1ll);
				new_component<gui_image_t>(goods.get_typ()->get_catg_symbol(), 0, ALIGN_CENTER_V, true);
				new_component<gui_colorbox_t>(goods.get_typ()->get_color())->set_size(GOODS_COLOR_BOX_SIZE);
				new_component<gui_label_t>(goods.get_typ()->get_name(), is_available ? SYSCOL_TEXT : SYSCOL_TEXT_WEAK);

				new_component<gui_factory_storage_bar_t>(&goods, pfactor, is_output);
				new_component<gui_factory_storage_label_t>(&goods, (uint32)goods.get_capacity(pfactor));
				if (!is_output && is_available) {
					add_table(2,1);
					{
						new_component<gui_image_t>(skinverwaltung_t::input_output ? skinverwaltung_t::in_transit->get_image_id(0):IMG_EMPTY, 0, 0, true);
						new_component<gui_factory_intransit_label_t>(&goods, pfactor);
					}
					end_table();
					new_component<gui_factory_monthly_prod_label_t>(fab, goods.get_typ(), pfactor, is_output);
				}
				else {
					new_component_span<gui_empty_t>(2);
				}

			}
		}
		end_table();
	}
	set_size(get_min_size());
}

void gui_factory_storage_info_t::draw(scr_coord offset)
{
	if (is_output) {
		lb_alert.set_visible(fab->get_total_out() >= fab->get_total_output_capacity());
	}
	else {
		lb_alert.set_visible(!fab->get_total_in());
	}

	gui_aligned_container_t::draw(offset);
}


gui_freight_halt_stat_t::gui_freight_halt_stat_t(halthandle_t halt)
{
	this->halt = halt;
	if (halt.is_bound()) {
		set_table_layout(5,1);
		new_component<gui_halt_capacity_bar_t>(halt,2)->set_width(LINESPACE*4); // todo: set width
		add_component(&label_name);
		old_month = -1; // force update
		lb_handling_amount.init(SYSCOL_TEXT, gui_label_t::right);
		lb_handling_amount.set_tooltip(translator::translate("The number of goods that handling at the stop last month"));
		new_component<gui_halt_handled_goods_images_t>(halt, true);
		add_component(&lb_handling_amount);
		bt_show_halt_network.init( button_t::roundbox, "Networks" );
		bt_show_halt_network.set_tooltip( translator::translate("Open the minimap window to show the freight network of this stop") );
		bt_show_halt_network.add_listener(this);
		if (skinverwaltung_t::open_window) {
			bt_show_halt_network.set_image(skinverwaltung_t::open_window->get_image_id(0));
			bt_show_halt_network.set_image_position_right(true);
		}
		add_component(&bt_show_halt_network);
	}
}

bool gui_freight_halt_stat_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if ( comp==&bt_show_halt_network ) {
		map_frame_t *win = dynamic_cast<map_frame_t*>(win_get_magic(magic_reliefmap));
		if (!win) {
			create_win(-1, -1, new map_frame_t(), w_info, magic_reliefmap);
			win = dynamic_cast<map_frame_t*>(win_get_magic(magic_reliefmap));
		}
		win->set_halt(halt, true);
		top_win(win);
		return true;
	}
	return false;
}

bool gui_freight_halt_stat_t::infowin_event(const event_t *ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);
	if (ev->mx < 0 || ev->mx > bt_show_halt_network.get_pos().x ) {
		return swallowed;
	}
	if(  !swallowed  &&  ev->my > 0 && ev->my < size.h &&  halt.is_bound()  ) {

		if(IS_LEFTRELEASE(ev)) {
			if(  event_get_last_control_shift() != 2  ) {
				halt->show_info();
			}
			return true;
		}
		if(  IS_RIGHTRELEASE(ev)  ) {
			world()->get_viewport()->change_world_position(halt->get_basis_pos3d());
			return true;
		}
	}
	return swallowed;
}

void gui_freight_halt_stat_t::draw(scr_coord offset)
{
	label_name.buf().append(halt->get_name());
	label_name.set_color(color_idx_to_rgb(halt->get_owner()->get_player_color1() + env_t::gui_player_color_dark));
	label_name.update();

	if (old_month != world()->get_current_month() % 12) {
		old_month = world()->get_current_month()%12;
		lb_handling_amount.buf().append(halt->get_finance_history(1, HALT_GOODS_HANDLING_VOLUME)/100.0, 1);
		lb_handling_amount.buf().append(translator::translate("tonnen"));

		lb_handling_amount.update();
	}

	set_size(get_size());
	if (win_get_magic(magic_halt_info + halt.get_id())) {
		display_blend_wh_rgb(offset.x + get_pos().x, offset.y + get_pos().y, get_size().w, get_size().h, SYSCOL_TEXT_HIGHLIGHT, 30);
	}
	gui_aligned_container_t::draw(offset);
}

gui_factory_nearby_halt_info_t::gui_factory_nearby_halt_info_t(fabrik_t *factory)
{
	this->fab = factory;
	set_table_layout(1,0);
	if (fab) {
		old_halt_count = fab->get_nearby_freight_halts().get_count();
		update_table();
	}
}

void gui_factory_nearby_halt_info_t::update_table()
{
	old_halt_count = fab->get_nearby_freight_halts().get_count();
	remove_all();
	FOR(const vector_tpl<nearby_halt_t>, freight_halt, fab->get_nearby_freight_halts()) {
		new_component<gui_freight_halt_stat_t>(freight_halt.halt);
	}
	if (!old_halt_count) {
		add_table(2,1);
		new_component<gui_image_t>(skinverwaltung_t::alerts ? skinverwaltung_t::alerts->get_image_id(2) : IMG_EMPTY, 0, ALIGN_CENTER_V, true);
		new_component<gui_label_t>("No connection to the freight stop.");
		end_table();
	}
	set_size(get_size());
}

void gui_factory_nearby_halt_info_t::draw(scr_coord offset)
{
	if (old_halt_count != fab->get_nearby_freight_halts().get_count()) {
		update_table();
	}
	gui_aligned_container_t::draw(offset);
}


void gui_goods_handled_factory_t::build_factory_list(const goods_desc_t *ware)
{
	factory_list.clear();
	FOR(vector_tpl<fabrik_t*>, const f, world()->get_fab_list()) {
		if (show_consumer) {
			// consume(accept) this?
			if(f->get_desc()->get_accepts_these_goods(ware)) {
				factory_list.append_unique(f->get_desc());
			}
		}
		else {
			// produce this?
			FOR(array_tpl<ware_production_t>, const& product, f->get_output()) {
				if (product.get_typ() == ware) {
					factory_list.append_unique(f->get_desc());
					break; // found
				}
			}
		}
	}
}

void gui_goods_handled_factory_t::draw(scr_coord offset)
{
	offset += pos;
	scr_coord_val xoff = 0;
	if (factory_list.get_count() > 0) {
		// if pakset has symbol, show symbol
		if (skinverwaltung_t::input_output){
			display_color_img(skinverwaltung_t::input_output->get_image_id(show_consumer ? 0:1), offset.x, offset.y + FIXED_SYMBOL_YOFF, 0, false, false);
			xoff += 14;
		}

		uint n = 0;
		FORX(vector_tpl<const factory_desc_t*>, f, factory_list, n++) {
			bool is_retired=false;
			if (world()->use_timeline()){
				if (f->get_building()->get_intro_year_month() > world()->get_timeline_year_month()) {
					// is future
					continue;
				}
				if (f->get_building()->get_retire_year_month() < world()->get_timeline_year_month()) {
					is_retired = true;
				}
			}
			if (n) {
				xoff += display_proportional_clip_rgb(offset.x + xoff, offset.y, ", ", ALIGN_LEFT, SYSCOL_TEXT, true);
			}
			buf.clear();
			buf.printf("%s", translator::translate(f->get_name()));
			xoff += display_proportional_clip_rgb(offset.x + xoff, offset.y, buf, ALIGN_LEFT, is_retired ? SYSCOL_OUT_OF_PRODUCTION : SYSCOL_TEXT, true);
		}

	}
	else {
		xoff += display_proportional_clip_rgb(offset.x + xoff, offset.y, "-", ALIGN_LEFT, COL_INACTIVE, true);
	}

	set_size(scr_size(xoff + D_H_SPACE * 2, D_LABEL_HEIGHT));
}
