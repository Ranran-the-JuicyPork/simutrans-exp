/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simcolor.h"
#include "../simevent.h"
#include "../display/simimg.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../sys/simsys.h"
#include "../simversion.h"
#include "../display/simgraph.h"
#include "../macros.h"
#include "../descriptor/skin_desc.h"
#include "../dataobj/environment.h"
#include "../dataobj/sve_cache.h"
#include "../simmenu.h"

#include "simwin.h"
#include "banner.h"
#include "welt.h"
#include "loadsave_frame.h"
#include "scenario_frame.h"
#include "server_frame.h"
#include "components/gui_image.h"
#include "optionen.h"


// Local adjustments
#define L_BANNER_ROWS        ( 5 )                             // Rows of scroll text
#define L_BANNER_TEXT_INDENT ( 4 )                             // Scroll text padding (left/right)
#define L_BANNER_HEIGHT      ( L_BANNER_ROWS * LINESPACE + 2 ) // Banner control height in pixels

#ifdef _OPTIMIZED
	#define L_DEBUG_TEXT " (optimized)"
#elif defined DEBUG
#	define L_DEBUG_TEXT " (debug)"
#else
#	define L_DEBUG_TEXT
#endif

// colors
#define COLOR_RAMP_SIZE ( 5 ) // Number or fade colors + normal color at index 0

// Banner color ramp
// Index 0 is the normal text color
static const uint8 colors[COLOR_RAMP_SIZE] = { COL_WHITE, COL_GREY3, COL_GREY4, COL_GREY5, COL_GREY6 };

class banner_text_t : public gui_component_t
{
	sint32 last_ms;
	int line;
public:
	banner_text_t() : last_ms(dr_time() - 70), line(0) {}

	scr_size get_min_size() const OVERRIDE
	{
		return scr_size(0, L_BANNER_HEIGHT);
	}

	void draw(scr_coord offset) OVERRIDE;
};

banner_t::banner_t() : gui_frame_t("")
{
	set_spacing(NO_SPACING);
	set_margin(scr_size(0, 0), scr_size(0, 0));
	set_table_layout(2,0);

	gui_aligned_container_t *menubar = add_table(1,0);
	menubar->set_table_frame(true, false);
	menubar->set_margin(scr_size(D_MARGIN_LEFT, D_MARGIN_TOP), scr_size(D_MARGIN_RIGHT, D_MARGIN_BOTTOM));
	{
		add_table(3,1);
		{
			new_component<gui_fill_t>();
			gameinfo_t gi(world());
			pakname_tooltip.append(gi.get_pak_name());
			new_component<gui_image_t>(skinverwaltung_t::logosymbol->get_image_id(0), 0, ALIGN_NONE, true)->set_tooltip(pakname_tooltip);
			new_component<gui_fill_t>();
		}
		end_table();

		new_component<gui_divider_t>();

		sve_cache_t::load_cache();
		const std::string last_save = sve_cache_t::get_most_recent_compatible_save();

		// Continue game button
		continue_game.init(button_t::roundbox | button_t::flexible, "Continue Game");
		continue_game.add_listener(this);
		continue_game.enable(!last_save.empty());

		if (!last_save.empty()) {
			continue_tooltip.printf(translator::translate("Load '%s'"), (last_save.c_str() + 5));
			continue_game.set_tooltip(continue_tooltip.get_str());
		}

		add_component(&continue_game);

		new_component<gui_divider_t>();

		// New game button
		new_map.init( button_t::roundbox | button_t::flexible, "Neue Karte");
		new_map.add_listener( this );
		add_component( &new_map );

		// Load game button
		load_map.init( button_t::roundbox | button_t::flexible, "Load game");
		load_map.add_listener( this );
		add_component( &load_map );

		// Play Tutorial
		//play_tutorial.init( button_t::roundbox | button_t::flexible, "Tutorial");
		//play_tutorial.add_listener( this );
		//if(  !(scenario_frame->check_file(pakset_tutorial.c_str(), ""))  ) {
		//	play_tutorial.disable();
		//	play_tutorial.set_tooltip("Scenario Tutorial is not available for this pakset. Please try loading Pak128.");
		//}
		//add_component( &play_tutorial );

		// Load scenario button
		load_scenario.init( button_t::roundbox | button_t::flexible, "Load scenario");
		load_scenario.add_listener( this );
		add_component( &load_scenario );

		// Play online button
		join_map.init( button_t::roundbox | button_t::flexible, "join game");
		join_map.add_listener( this );
		add_component( &join_map );

		// Options button
		options.init( button_t::roundbox | button_t::flexible, "Optionen");
		options.add_listener( this );
		add_component( &options );

		// Install button
		//install.init( button_t::roundbox | button_t::flexible, "Install");
		//install.add_listener( this );
		//add_component( &install );

		new_component<gui_fill_t>(false, true);
		new_component<gui_divider_t>();

		// Quit button
		quit.init( button_t::roundbox | button_t::flexible, "Beenden");
		quit.add_listener( this );
		add_component( &quit );
	}
	end_table();

	gui_aligned_container_t *tbl = add_table(1,0);
	tbl->set_table_frame(false, true);
	tbl->set_margin(scr_size(D_MARGIN_LEFT, D_MARGIN_TOP), scr_size(D_MARGIN_RIGHT, D_MARGIN_BOTTOM));
	{
		new_component<gui_label_t>("Welcome to Simutrans-Extended (formerly Simutrans-Experimental)", SYSCOL_TEXT_TITLE, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);

		add_table(1,0);
		{
			add_table(3,0);
			{
				new_component<gui_fill_t>();
				add_table(1,0);

				new_component<gui_label_t>("This is an extended version of Simutrans", SYSCOL_TEXT_TITLE, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);

				new_component<gui_label_t>("Developed by the Simutrans community", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);

				new_component<gui_label_t>("under the Artistic License", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);

				new_component<gui_label_t>("forked from Simutrans-Standard " QUOTEME(SIM_VERSION_MAJOR), SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);

				end_table();
				new_component<gui_fill_t>();
			}
			end_table();
		}
		end_table();

		add_table(1,0);
		{
			gui_label_t *lb = new_component<gui_label_t>("Selling of the program is forbidden.", color_idx_to_rgb(COL_ORANGE), gui_label_t::left);
			lb->set_shadow(SYSCOL_TEXT_SHADOW, true);
			lb->set_underline(true);
			new_component<gui_label_t>("For questions and support please visit:", SYSCOL_TEXT_TITLE, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
		}
		end_table();

		add_table(3,0);
		{
			new_component<gui_fill_t>();
			add_table(1,0);
			new_component<gui_label_t>("https://www.simutrans.com", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
			new_component<gui_label_t>("https://forum.simutrans.com", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
			new_component<gui_label_t>("https://wiki.simutrans.com", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
			end_table();
			new_component<gui_fill_t>();
		}
		end_table();

		new_component<gui_fill_t>(false, true);
		new_component<banner_text_t>();
		new_component<gui_fill_t>(false, true);
#ifdef REVISION
		new_component<gui_label_t>("Version " VERSION_NUMBER " " EXTENDED_VERSION " ", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
		new_component<gui_label_t>(VERSION_DATE " #" QUOTEME(REVISION), SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
#else
		new_component<gui_label_t>("Version " VERSION_NUMBER " " EXTENDED_VERSION " " VERSION_DATE, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left)->set_shadow(SYSCOL_TEXT_SHADOW, true);
#endif
	}
	end_table();

	reset_min_windowsize();
}


bool banner_t::infowin_event(const event_t *ev)
{
	if(  gui_frame_t::is_hit( ev->click_pos.x, ev->click_pos.y  )  ) {
		gui_frame_t::infowin_event( ev );
	}
	if (ev->ev_class == EVENT_SYSTEM && ev->ev_code == SYSTEM_QUIT) {
		env_t::quit_simutrans = true;
	}
	return false;
}


bool banner_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	if(  comp == &quit  ) {
		env_t::quit_simutrans = true;
		destroy_all_win(true);
	}
	else if(  comp == &continue_game  ) {
		sve_cache_t::load_cache();
		const std::string last_save = sve_cache_t::get_most_recent_compatible_save();
		destroy_all_win(true);
		welt->load(last_save.c_str());
	}
	else if(  comp == &new_map  ) {
		destroy_all_win(true);
		create_win( new welt_gui_t(&env_t::default_settings), w_info, magic_welt_gui_t );
	}
	else if(  comp == &load_map  ) {
		destroy_all_win(true);
		create_win( new loadsave_frame_t(true), w_info, magic_load_t);
	}
	else if(  comp == &load_scenario  ) {
		destroy_all_win(true);
		create_win( new scenario_frame_t(), w_info, magic_load_t );
	}
	else if(  comp == &options  ) {
		destroy_all_win(true);
		create_win( new optionen_gui_t(), w_info, magic_optionen_gui_t );
	}
	else if(  comp == &join_map  ) {
		destroy_all_win(true);
		create_win( new server_frame_t(), w_info, magic_server_frame_t );
	}
	return true;
}

void banner_t::show_banner()
{
	tool_t::simple_tool[TOOL_QUIT]->set_default_param("n");
	welt->set_tool(tool_t::simple_tool[TOOL_QUIT], NULL);
	tool_t::simple_tool[TOOL_QUIT]->set_default_param(0);
}


void banner_text_t::draw(scr_coord offset)
{
	// now the scrolling
	//                    BANNER_ROWS defines size and how many rows of text are shown in banner
	//                    BANNER_TEXT_INDENT defines left and right padding inside banner area

	static const char* const scrolltext[] = {
		#include "../scrolltext.h"
	};

	scr_coord cursor(get_pos() + offset);
	const scr_coord_val text_line = (line / LINESPACE) * 2;
	const scr_coord_val text_offset = line % LINESPACE;
	const scr_coord_val left = cursor.x;
	const scr_coord_val width = size.w;
	const scr_coord_val height = size.h;


	PUSH_CLIP_FIT( left, cursor.y, width, height );

	display_fillbox_wh_clip_rgb(left, cursor.y,          width, height, color_idx_to_rgb(COL_GREY1), true);
	display_fillbox_wh_clip_rgb(left, cursor.y - 1,      width, 1,      color_idx_to_rgb(COL_GREY3), false);
	display_fillbox_wh_clip_rgb(left, cursor.y + height, width, 1,      color_idx_to_rgb(COL_GREY6), false);

	cursor.y++;

	for(  int row = 0;  row < L_BANNER_ROWS+1;  row++  ) {

		PIXVAL color;
		if(  row > L_BANNER_ROWS-COLOR_RAMP_SIZE+1  ) {
			color = color_idx_to_rgb(colors[L_BANNER_ROWS-row+1]);
		}
		else {
			color = color_idx_to_rgb(colors[0]);
		}

		if (scrolltext[text_line + row * 2] == NULL || scrolltext[text_line + row * 2 + 1] == NULL)
		{
			break;
		}

		display_proportional_clip_rgb( left + L_BANNER_TEXT_INDENT,         cursor.y - text_offset, scrolltext[text_line + row*2    ], ALIGN_LEFT,  color, false);
		display_proportional_clip_rgb( left + width - L_BANNER_TEXT_INDENT, cursor.y - text_offset, scrolltext[text_line + row*2 + 1], ALIGN_RIGHT, color, false);

		cursor.y += LINESPACE;
	}

	POP_CLIP();

	// scroll on every 70 ms
	if(  dr_time() > (last_ms + 70u)  ) {
		last_ms += 70u;
		line ++;
	}

	if (scrolltext[text_line + (L_BANNER_ROWS+1)*2] == 0) {
		line = 0;
	}

}
