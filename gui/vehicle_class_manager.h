/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_VEHICLE_CLASS_MANAGER_H
#define GUI_VEHICLE_CLASS_MANAGER_H


/*
 * Convoi details window
 */

#include "gui_frame.h"
#include "simwin.h"
#include "components/gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_speedbar.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_combobox.h"
#include "components/action_listener.h"
#include "../convoihandle_t.h"

#include "../vehicle/simvehicle.h"

class scr_coord;


class gui_class_vehicleinfo_t : public gui_container_t/*, private action_listener_t*/
{
private:
	/**
	 * Handle the convoi to be displayed.
	 */
	convoihandle_t cnv;


	// When comboboxes eventually makes it to this part of the window....
	//slist_tpl<gui_combobox_t *> pass_class_sel;
	//slist_tpl<gui_combobox_t *> mail_class_sel;

	gui_container_t cont;

	// below a duplication of code, for the moment until I can get it automatically
	//char *class_name;
	//char *pass_class_name_untranslated[32];
	//char *mail_class_name_untranslated[32];

public:
	gui_class_vehicleinfo_t(convoihandle_t cnv);



	//bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void set_cnv( convoihandle_t c ) { cnv = c; }

	void draw(scr_coord offset);
};



class vehicle_class_manager_t : public gui_frame_t , private action_listener_t
{
public:
	enum sort_mode_t { by_destination=0, by_via=1, by_amount_via=2, by_amount=3, SORT_MODES=4 };

private:

	gui_class_vehicleinfo_t veh_info;
	gui_scrollpane_t scrolly;

	convoihandle_t cnv;
	button_t reset_all_classes_button;

	vector_tpl<char> class_indices;

	slist_tpl<gui_combobox_t *> pass_class_sel;
	slist_tpl<gui_combobox_t *> mail_class_sel;

	sint16 button_width = 190;

	gui_container_t cont;

	uint16 current_number_of_classes;
	uint16 current_number_of_accommodations;
	uint32 current_number_of_vehicles;

	uint8 highest_catering;
	bool is_tpo;

	uint8 vehicle_count;
	uint8 old_vehicle_count;

	uint16 header_height;

	uint32 overcrowded_capacity;

	char *pass_class_name_untranslated[32];
	char *mail_class_name_untranslated[32];

	uint32 pass_capacity_at_class[255] = { 0 };
	uint32 mail_capacity_at_class[255] = { 0 };

	uint32 pass_capacity_at_accommodation[255] = { 0 };
	uint32 mail_capacity_at_accommodation[255] = { 0 };

	bool any_pass = false;
	bool any_mail = false;



public:

	vehicle_class_manager_t(convoihandle_t cnv);

	/**
	* Do the dynamic component layout
	*/
	void layout(scr_coord pos);

	/**
	* Build the class lists
	*/

	void build_class_entries();

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	const char * get_help_filename() const OVERRIDE {return "vehicle_class_manager.txt"; }

	virtual void set_windowsize(scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * called when convoi was renamed
	 */
	void update_data() { set_dirty(); }

	// this constructor is only used during loading
	vehicle_class_manager_t();

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_class_manager; }

	~vehicle_class_manager_t();
};


class gui_convoy_fare_class_changer_t : public gui_aligned_container_t, private action_listener_t
{
	convoihandle_t cnv;

	bool any_class = false;

	uint8 old_vehicle_count=0;
	bool old_reversed=false;
	uint8 old_player_nr;

	// expand/collapse things
	gui_aligned_container_t cont_accommodation_table;
	gui_label_t lb_collapsed;
	button_t show_hide_accommodation_table;
	bool show_accommodation_table = false;

	gui_aligned_container_t cont_vehicle_table;

	button_t init_class;
	void reset_fare_class();

	void update_vehicles();
	void add_accommodation_rows(uint8 catg);

public:
	gui_convoy_fare_class_changer_t(convoihandle_t cnv);

	//void set_cnv(convoihandle_t c) { cnv = c; }

	void draw(scr_coord offset) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};


class gui_cabin_fare_changer_t : public gui_aligned_container_t, private action_listener_t
{
	vehicle_t *vehicle;
	uint8 cabin_class;

	button_t buttons[5];
	char *class_name_untranslated[5];

public:
	gui_cabin_fare_changer_t(vehicle_t *v, uint8 original_class);
	void draw(scr_coord offset) OVERRIDE;
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

class gui_accommodation_fare_changer_t : public gui_aligned_container_t, private action_listener_t
{
	convoihandle_t cnv;
	linehandle_t line;
	uint8 catg;
	uint8 accommodation_class;
	uint8 fare_class; // 255 = assigned multiple

	sint8 player_nr;

	sint16 old_temp = 0;

	button_t buttons[5];
	char *class_name_untranslated[5];

	void set_class_reassignment(convoihandle_t target_convoy);
	void update_button_state();

public:
	gui_accommodation_fare_changer_t(linehandle_t line, convoihandle_t cnv, uint8 goods_catg_index, uint8 original_class=0, uint8 current_fare_class=0);
	void draw(scr_coord offset) OVERRIDE;
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};


#endif
