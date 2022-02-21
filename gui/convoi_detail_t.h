/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CONVOI_DETAIL_T_H
#define GUI_CONVOI_DETAIL_T_H


#include "gui_frame.h"
#include "components/gui_aligned_container.h"
#include "components/gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_speedbar.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_combobox.h"
#include "components/gui_tab_panel.h"
#include "components/action_listener.h"
#include "components/gui_convoy_formation.h"
#include "components/gui_chart.h"
#include "components/gui_button_to_chart.h"
#include "../convoihandle_t.h"
#include "../vehicle/rail_vehicle.h"
#include "simwin.h"

class scr_coord;

#define MAX_ACCEL_CURVES 4
#define MAX_FORCE_CURVES 2
#define MAX_PHYSICS_CURVES (MAX_ACCEL_CURVES+MAX_FORCE_CURVES)
#define SPEED_RECORDS 25


// Band graph showing loading status based on capacity for each vehicle "accommodation class"
// The color is based on the color of the cargo
class gui_capacity_occupancy_bar_t : public gui_container_t
{
	vehicle_t *veh;
	uint8 a_class;
	bool size_fixed = true;

public:
	gui_capacity_occupancy_bar_t(vehicle_t *v, uint8 accommo_class=0);

	scr_size get_min_size() const OVERRIDE;
	scr_size get_max_size() const OVERRIDE;

	void set_size_fixed(bool yesno) { size_fixed = yesno; };

	void display_loading_bar(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, uint16 loading, uint16 capacity, uint16 overcrowd_capacity);
	void draw(scr_coord offset) OVERRIDE;
};

class gui_vehicle_cargo_info_t : public gui_aligned_container_t
{
	// Note: different from freight_list_sorter_t
	enum filter_mode_t : uint8 {
		hide_detail = 0,
		by_unload_halt,      // (by wealth)
		by_destination_halt, // (by wealth)
		by_final_destination // (by wealth)
	};

	schedule_t * schedule;
	vehicle_t *veh;
	uint16 total_cargo=0;
	uint8 show_loaded_detail = by_unload_halt;

public:
	gui_vehicle_cargo_info_t(vehicle_t *v, uint8 display_loaded_detail);

	void draw(scr_coord offset) OVERRIDE;

	void update();
};


// helper class to show the colored acceleration text
class gui_acceleration_label_t : public gui_container_t
{
private:
	convoihandle_t cnv;
public:
	gui_acceleration_label_t(convoihandle_t cnv);

	scr_size get_min_size() const OVERRIDE { return get_size(); };
	scr_size get_max_size() const OVERRIDE { return get_min_size(); }

	void draw(scr_coord offset) OVERRIDE;
};

class gui_acceleration_time_label_t : public gui_container_t
{
private:
	convoihandle_t cnv;
public:
	gui_acceleration_time_label_t(convoihandle_t cnv);

	scr_size get_min_size() const OVERRIDE { return get_size(); };
	scr_size get_max_size() const OVERRIDE { return get_min_size(); }

	void draw(scr_coord offset) OVERRIDE;
};

class gui_acceleration_dist_label_t : public gui_container_t
{
private:
	convoihandle_t cnv;
public:
	gui_acceleration_dist_label_t(convoihandle_t cnv);

	scr_size get_min_size() const OVERRIDE { return get_size(); };
	scr_size get_max_size() const OVERRIDE { return get_min_size(); }

	void draw(scr_coord offset) OVERRIDE;
};

/**
 * One element of the vehicle list display
 */
class gui_vehicleinfo_t : public gui_container_t
{
private:
	convoihandle_t cnv;

	gui_combobox_t class_selector;

	slist_tpl<gui_combobox_t *> class_selectors;

	vector_tpl<uint16> class_indices;

public:
	gui_vehicleinfo_t(convoihandle_t cnv);

	void set_cnv( convoihandle_t c ) { cnv = c; }

	void draw(scr_coord offset);
};

// content of payload info tab
class gui_convoy_payload_info_t : public gui_aligned_container_t
{
private:
	convoihandle_t cnv;
	uint8 display_mode = 1; // by_unload_halt

public:
	gui_convoy_payload_info_t(convoihandle_t cnv);

	void set_cnv(convoihandle_t c) { cnv = c; }
	void set_display_mode(uint8 mode) { display_mode = mode; update_list(); }

	void update_list();

	void draw(scr_coord offset);
};

// content of maintenance info tab
class gui_convoy_maintenance_info_t : public gui_container_t
{
private:
	convoihandle_t cnv;
	bool any_obsoletes;

public:
	gui_convoy_maintenance_info_t(convoihandle_t cnv);

	void set_cnv(convoihandle_t c) { cnv = c; }

	void draw(scr_coord offset);
};

/**
 * Displays an information window for a convoi
 */
class convoi_detail_t : public gui_frame_t , private action_listener_t
{
public:
	enum sort_mode_t {
		by_destination = 0,
		by_via         = 1,
		by_amount_via  = 2,
		by_amount      = 3,
		SORT_MODES     = 4
	};

private:
	gui_aligned_container_t cont_maintenance, cont_payload_tab;

	convoihandle_t cnv;

	gui_vehicleinfo_t veh_info;
	gui_convoy_formation_t formation;
	gui_convoy_payload_info_t cont_payload_info;
	gui_convoy_maintenance_info_t maintenance;
	gui_aligned_container_t cont_accel, cont_force;
	gui_chart_t accel_chart, force_chart;

	gui_scrollpane_t scrolly_formation;
	gui_scrollpane_t scrolly;
	gui_scrollpane_t scrolly_payload_info;
	gui_scrollpane_t scrolly_maintenance;

	static sint16 tabstate;
	gui_tab_panel_t switch_chart;
	gui_tab_panel_t tabs;

	gui_aligned_container_t container_chart;
	button_t sale_button;
	button_t withdraw_button;
	button_t retire_button;
	button_t class_management_button;
	gui_combobox_t cb_loaded_detail;

	gui_combobox_t overview_selctor;
	gui_label_buf_t
		lb_vehicle_count,                   // for main frame
		lb_loading_time, lb_catering_level, // for payload tab
		lb_odometer, lb_value;              // for maintenance tab
	gui_acceleration_label_t      *lb_acceleration;
	gui_acceleration_time_label_t *lb_acc_time;
	gui_acceleration_dist_label_t *lb_acc_distance;

	gui_button_to_chart_array_t btn_to_accel_chart, btn_to_force_chart; //button_to_chart

	sint64 accel_curves[SPEED_RECORDS][MAX_ACCEL_CURVES];
	sint64 force_curves[SPEED_RECORDS][MAX_FORCE_CURVES];
	uint8 te_curve_abort_x = SPEED_RECORDS;

	sint64 old_seed = 0; // gui update flag

	void update_labels();

	void init(convoihandle_t cnv);

	void set_tab_opened();

public:
	convoi_detail_t(convoihandle_t cnv = convoihandle_t());

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	const char * get_help_filename() const OVERRIDE {return "convoidetail.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * called when convoi was renamed
	 */
	void update_data() { set_dirty(); }

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_convoi_detail; }
};

#endif
