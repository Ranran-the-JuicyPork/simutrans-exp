/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "siminteraction.h"

#include "simversion.h"
#include "dataobj/environment.h"
#include "gui/gui_frame.h"
#include "gui/help_frame.h"
#include "network/network_cmd_ingame.h"
#include "dataobj/scenario.h"
#include "simevent.h"
#include "utils/simrandom.h"
#include "simmenu.h"
#include "player/simplay.h"
#include "simsound.h"
#include "sys/simsys.h"
#include "simticker.h"
#include "gui/simwin.h"
#include "simworld.h"
#include "descriptor/sound_desc.h"
#include "obj/zeiger.h"
#include "display/viewport.h"


karte_ptr_t interaction_t::world;

void interaction_t::move_view( const event_t &ev )
{
	koord new_ij = viewport->get_world_position();

	sint16 new_xoff = viewport->get_x_off() - (ev.mouse_pos.x - ev.click_pos.x) * env_t::scroll_multi;
	sint16 new_yoff = viewport->get_y_off() - (ev.mouse_pos.y - ev.click_pos.y) * env_t::scroll_multi;

	// this sets the new position and mark screen dirty
	// => with next refresh we will be at a new location
	viewport->change_world_position( new_ij, new_xoff, new_yoff );

	// move the mouse pointer back to starting location => infinite mouse movement
	if ((ev.mouse_pos.x - ev.click_pos.x) != 0 || (ev.mouse_pos.y - ev.click_pos.y) != 0) {
		if(!env_t::scroll_infinite  ||  !move_pointer(ev.click_pos.x, ev.click_pos.y)) {
			// fails in finger mode
			change_drag_start(ev.mouse_pos - ev.click_pos);
		}
	}
}


void interaction_t::move_cursor( const event_t &ev )
{
	zeiger_t *zeiger = world->get_zeiger();

	if(! zeiger ) {
		// No cursor to move, exit
		return;
	}

	static int mb_alt=0;

	tool_t *tool = world->get_tool(world->get_active_player_nr());

	const koord3d pos = viewport->get_new_cursor_position(scr_coord(ev.mouse_pos.x,ev.mouse_pos.y), tool->is_grid_tool());

	if( pos == koord3d::invalid ) {
		zeiger->change_pos(pos);
		return;
	}

	// move cursor
	const koord3d prev_pos = zeiger->get_pos();
	if(  (prev_pos != pos ||  ev.button_state != mb_alt)  ) {

		mb_alt = ev.button_state;

		zeiger->change_pos(pos);

		if (!tool->move_has_effects()) {
			tool->end_move(world->get_active_player(), pos);
			is_dragging = false;

		}
		else if(  !env_t::networkmode  ||  tool->is_move_network_safe(world->get_active_player())) {
			tool->flags = (event_get_last_control_shift()^tool_t::control_invert) | tool_t::WFL_LOCAL;
			if(tool->check_pos( world->get_active_player(), zeiger->get_pos() )==NULL) {
				if(  ev.button_state == 0  ) {
					tool->end_move(world->get_active_player(), pos);
					is_dragging = false;
				}
				else if(ev.ev_class==EVENT_DRAG) {
					if(!is_dragging  &&  prev_pos != koord3d::invalid  &&  tool->check_pos( world->get_active_player(), prev_pos )==NULL) {
						const char* err = world->get_scenario()->is_work_allowed_here(world->get_active_player(), tool->get_id(), tool->get_waytype(), prev_pos);
						if (err == NULL) {
							tool->begin_move(world->get_active_player(), pos);
							is_dragging = true;
						}
						else {
							tool->end_move(world->get_active_player(), pos);
							is_dragging = false;
						}
					}
				}
				if (is_dragging) {
					const char* err = world->get_scenario()->is_work_allowed_here(world->get_active_player(), tool->get_id(), tool->get_waytype(), pos);
					if (err == NULL) {
						tool->move( world->get_active_player(), is_dragging, pos );
					}
				}
			}
			tool->flags = 0;
		}

		if(  (ev.button_state&7)==0  ) {
			// time, since mouse got here
			world->set_mouse_rest_time(dr_time());
			world->set_sound_wait_time(AMBIENT_SOUND_INTERVALL); // 13s no movement: play sound
		}
	}
}


void interaction_t::interactive_event( const event_t &ev )
{
	if(ev.ev_class == EVENT_KEYBOARD) {
		DBG_MESSAGE("interaction_t::interactive_event()","Keyboard event with code %d '%c'", ev.ev_code, (ev.ev_code>=32  &&  ev.ev_code<=126) ? ev.ev_code : '?' );

		switch(ev.ev_code) {

			// cursor movements
			case SIM_KEY_UPRIGHT:
				viewport->change_world_position(viewport->get_world_position() + koord::north);
				world->set_dirty();
				break;
			case SIM_KEY_DOWNLEFT:
				viewport->change_world_position(viewport->get_world_position() + koord::south);
				world->set_dirty();
				break;
			case SIM_KEY_UPLEFT:
				viewport->change_world_position(viewport->get_world_position() + koord::west);
				world->set_dirty();
				break;
			case SIM_KEY_DOWNRIGHT:
				viewport->change_world_position(viewport->get_world_position() + koord::east);
				world->set_dirty();
				break;
			case SIM_KEY_RIGHT:
				viewport->change_world_position(viewport->get_world_position() + koord(+1,-1));
				world->set_dirty();
				break;
			case SIM_KEY_DOWN:
				viewport->change_world_position(viewport->get_world_position() + koord(+1,+1));
				world->set_dirty();
				break;
			case SIM_KEY_UP:
				viewport->change_world_position(viewport->get_world_position() + koord(-1,-1));
				world->set_dirty();
				break;
			case SIM_KEY_LEFT:
				viewport->change_world_position(viewport->get_world_position() + koord(-1,+1));
				world->set_dirty();
				break;

			// closing windows
			case 27:
			case 127:
				if( !IS_CONTROL_PRESSED( &ev ) && !IS_SHIFT_PRESSED( &ev ) ) {
					// close topmost win
					destroy_win( win_get_top(), false );
				}
				break;

			case SIM_KEY_F1:
				if(  gui_frame_t *win = win_get_top()  ) {
					if(  const char *helpfile = win->get_help_filename()  ) {
						help_frame_t::open_help_on( helpfile );
						break;
					}
				}
				world->set_tool( tool_t::dialog_tool[DIALOG_HELP], world->get_active_player() );
				break;

			// just ignore the key
			case 0:
				break;

			// distinguish between backspace and ctrl-H (both keycode==8), and enter and ctrl-M (both keycode==13)
			case 8:
			case 13:
				if(  !IS_CONTROL_PRESSED(&ev)  &&  !IS_SHIFT_PRESSED(&ev)  ) {
					// Control is _not_ pressed => Backspace or Enter pressed.
					if(  ev.ev_code == 8  ) {
						// Backspace
						sound_play(SFX_SELECT,255,TOOL_SOUND);
						destroy_all_win(false);
					}
					// Ignore Enter and Backspace but not Ctrl-H and Ctrl-M
					break;
				}
				/* FALLTHROUGH */

			default:
				{
					bool ok=false;
					for(tool_t* const i : tool_t::char_to_tool) {
						if(  i->command_key == ev.ev_code  ) {
							if(  i->command_flags == 0  ||  (ev.ev_key_mod & (SIM_MOD_SHIFT|SIM_MOD_CTRL)) == i->command_flags  ) {
								world->set_tool(i, world->get_active_player());
								ok = true;
								break;
							}
						}
					}
#ifdef STEAM_BUILT
					// Block F12 from bringing up Keyboard Help (for Steam Screenshot) - but still allow F12 to be used if defined in pakset
					if (ev.ev_code==SIM_KEY_F12) {
						ok=true;
					}
#endif
					if(!ok) {
						help_frame_t::open_help_on( "keys.txt" );
					}
				}
				break;
		}
	}

	if(  IS_LEFTRELEASE(&ev)  &&  ev.mouse_pos.y < display_get_height() -16 -(TICKER_HEIGHT*ticker::empty())  ) {

		DBG_MESSAGE("interaction_t::interactive_event(event_t &ev)", "calling a tool");

		koord3d pos = world->get_zeiger()->get_pos();
		if(world->is_within_grid_limits(pos.get_2d())) {

			bool suspended = false; // true if execution was suspended, i.e. sent to server
			tool_t *tool = world->get_tool(world->get_active_player_nr());
			player_t *player = world->get_active_player();
			tool->flags = event_get_last_control_shift()^tool_t::control_invert;
			// first check for visibility etc (needs already right flags)
			const char *err = tool->check_pos( player, pos );
			if (err==NULL) {
				err = world->call_work(tool, player, pos, suspended);
			}
			if (!suspended) {
				// play sound / error message
				world->get_active_player()->tell_tool_result(tool, pos, err);

				// Check if we need to update pointer(zeiger) position.
				if( err == NULL  &&  tool->update_pos_after_use() ) {
					// Cursor might need movement (screen has changed, we get a new one under the mouse pointer)
					const koord3d pos_new = viewport->get_new_cursor_position(scr_coord(ev.mouse_pos.x,ev.mouse_pos.y), tool->is_grid_tool());
					world->get_zeiger()->set_pos(pos_new);
				}
			}
			tool->flags = 0;
		}
	}

	// mouse wheel scrolled -> rezoom
	if (ev.ev_class == EVENT_CLICK) {

		// first, we need to check cursor is valid, won't zoom otherwise

		const koord3d cursor_pos = world->get_zeiger()->get_pos();

		if( cursor_pos == koord3d::invalid) {
			//ignore event
			return;
		}

		bool zoom_successful = false;

		// store old screen position of centered tile
		scr_coord s = viewport->get_screen_coord(cursor_pos, koord(0,0));

		if(ev.ev_code==MOUSE_WHEELUP) {
			if(win_change_zoom_factor(true)) {
				zoom_successful = true;
			}
		}
		else if(ev.ev_code==MOUSE_WHEELDOWN) {
			if(win_change_zoom_factor(false)) {
				zoom_successful = true;
			}
		}

		// zoom can fail if we are max zoomed in/out, so:
		if (zoom_successful) {
			// calculate offsets such that tile under cursor is still on the same screen position
			viewport->change_world_position(cursor_pos, koord(0,0), s);

			//and move cursor to the new position under the mouse
			move_cursor(ev);

			world->set_dirty();
		}
	}
}


bool interaction_t::process_event( event_t &ev )
{
	if(ev.ev_class==EVENT_SYSTEM  &&  ev.ev_code==SYSTEM_QUIT) {
		// since we run in a sync_step, quitting may be needed to be delagated to a tool
		if(  (LOAD_RANDOM | MAP_CREATE_RANDOM | MODAL_RANDOM) & get_random_mode()  ) {
			// next sync step would take tool long
			env_t::quit_simutrans = true;
		}
		else {
			// we call the proper tool for quitting
			world->set_tool(tool_t::simple_tool[TOOL_QUIT], NULL);
		}

		// we may be requested to save the game before exit
		if(  env_t::server  &&  env_t::server_save_game_on_quit  ) {

			// to ensure only one attempt is made
			env_t::server_save_game_on_quit = false;

			// following code quite similar to nwc_sync_t::do_coomand
			dr_chdir( env_t::user_dir );

			// first save password hashes
			char fn[256];
			sprintf( fn, "server%d-pwdhash.sve", env_t::server );
			loadsave_t file;
			if(file.wr_open(fn, loadsave_t::zipped, 1, "hashes", SAVEGAME_VER_NR, EXTENDED_VER_NR, EXTENDED_REVISION_NR) == loadsave_t::FILE_STATUS_OK) {
				world->rdwr_player_password_hashes( &file );
				file.close();
			}

			// remove passwords before transfer on the server and set default client mask
			// they will be restored in karte_t::load
			uint16 unlocked_players = 0;
			for(  int i=0;  i<PLAYER_UNOWNED; i++  ) {
				player_t *player = world->get_player(i);
				if(  player==NULL  ||  player->access_password_hash().empty()  ) {
					unlocked_players |= (1<<i);
				}
				else {
					player->access_password_hash().clear();
				}
			}

			// save game
			sprintf( fn, "server%d-restore.sve", env_t::server );
			bool old_restore_UI = env_t::restore_UI;
			env_t::restore_UI = true;
			world->save( fn, false, SERVER_SAVEGAME_VER_NR, EXTENDED_VER_NR, EXTENDED_REVISION_NR, false);
			env_t::restore_UI = old_restore_UI;
		}
		else if(  env_t::reload_and_save_on_quit  &&  !env_t::networkmode  ) {
			// save current game, if not online
			bool old_restore_UI = env_t::restore_UI;
			env_t::restore_UI = true;

			// construct from pak name an autosave if requested
			std::string pak_name( "autosave-" );
			pak_name.append( env_t::objfilename );
			pak_name.erase( pak_name.length()-1 );
			pak_name.append( ".sve" );

			world->save( pak_name.c_str(), true, SAVEGAME_VER_NR, EXTENDED_VER_NR, EXTENDED_REVISION_NR, false);
			env_t::restore_UI = old_restore_UI;
		}
		destroy_all_win(true);
		return true;
	}

	if(ev.ev_class==IGNORE_EVENT) {
		// ignore it
		return false;
	}

	DBG_DEBUG4("interaction_t::process_event", "calling check_pos_win");
	if(check_pos_win(&ev)){
		// The event is shallowed by the GUI, next.
		return false;
	}

	DBG_DEBUG4("interaction_t::process_event", "after check_pos_win");

	// Handle map drag with right-click

	static bool left_drag = false;

	if(IS_RIGHTCLICK(&ev)) {
		display_show_pointer(false);
	}
	else if(IS_RIGHTRELEASE(&ev)) {
		display_show_pointer(true);
	}
	else if(IS_RIGHTDRAG(&ev)) {
		// unset following
		world->get_viewport()->set_follow_convoi( convoihandle_t() );
		catch_dragging();
		move_view(ev);
	}
	else if ((left_drag || (world->get_tool(world->get_active_player_nr())->get_id() & GENERAL_TOOL) != 0) && IS_LEFTDRAG(&ev)) {
		/* ok, we have a general tool selected, and we have a left drag or left release event with an actual difference
		 * => move the map, if we are beyond a threshold */
		if(  left_drag  ||  abs(ev.click_pos.x - ev.mouse_pos.x)+abs(ev.click_pos.y - ev.mouse_pos.y)>=env_t::scroll_threshold  ) {
			if (!left_drag) {
				display_show_pointer(false);
				left_drag = true;
			}
			world->get_viewport()->set_follow_convoi(convoihandle_t());
			catch_dragging();
			move_view(ev);
			ev.ev_code = IGNORE_EVENT;
		}
	}

	if(  IS_LEFTRELEASE(&ev)  &&  left_drag  ) {
		// show the mouse and swallow this event if we were dragging before
		ev.ev_code = IGNORE_EVENT;
		display_show_pointer(true);
		left_drag = false;
	}


	DBG_DEBUG4("interaction_t::process_event", "check if cursor needs movement");


	if( (ev.ev_class==EVENT_DRAG  &&  ev.ev_code==MOUSE_LEFTBUTTON)  ||  (ev.button_state==0  &&  ev.ev_class==EVENT_MOVE)  ||  ev.ev_class==EVENT_RELEASE) {
		move_cursor(ev);
	}

	DBG_DEBUG4("interaction_t::process_event", "calling interactive_event");

	interactive_event(ev);

	DBG_DEBUG4("interaction_t::process_event", "end of event handling");

	return false;
}


void interaction_t::check_events()
{
	event_t ev;

	if(  env_t::networkmode  ) {
		set_random_mode( INTERACTIVE_RANDOM );
	}

	win_poll_event(&ev);

	event_t deferred_ev;
	deferred_ev.ev_class = EVENT_NONE;

	for(  int swallow_dummies=0;  swallow_dummies<4;  swallow_dummies++  ) {

		/* there are always dummy events in the queue, usually less than 4
		 * => retry a few times until we get a useful event
		 * Otherwise very laggy response at low fps
		 */
		if (ev.ev_class != EVENT_NONE) {
			DBG_DEBUG4("interaction_t::check_events", "event %d after %d dummies", ev.ev_class, swallow_dummies);
			swallow_dummies = 0;

			if (ev.ev_class == EVENT_DRAG) {
				// defer processing, since there might be many triggered at once
				// Otherwise mark tiles could be alled twice duing one step
				deferred_ev = ev;
			}
			else {
				// still one drag left in queue?
				if (deferred_ev.ev_class == EVENT_DRAG) {
					// do this first
					process_event(deferred_ev);
					deferred_ev.ev_class = EVENT_NONE;
				}

				if (process_event(ev)) {
					// We have been asked to stop processing, exit.
					return;
				}
			}
		}

		win_poll_event(&ev);
	}

	if (deferred_ev.ev_class == EVENT_DRAG) {
		// process pending drag events
		process_event(deferred_ev);
		deferred_ev.ev_class = EVENT_NONE;
	}

	if(  env_t::networkmode  ) {
		clear_random_mode( INTERACTIVE_RANDOM );
	}
}


interaction_t::interaction_t()
{
	viewport = world->get_viewport();
	is_dragging = false;

	// Requires a world with a view already attached!
	assert(viewport);
}
