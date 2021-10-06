/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "pier_builder.h"
#include "../descriptor/pier_desc.h"
#include "../obj/pier.h"
#include "../boden/pier_deck.h"

#include "../simtool.h"

#include "../dataobj/scenario.h"
#include "../gui/tool_selector.h"
#include "../obj/gebaeude.h"

karte_ptr_t pier_builder_t::welt;

stringhashtable_tpl<pier_desc_t *, N_BAGS_MEDIUM> pier_builder_t::desc_table;

static bool compare_piers(const pier_desc_t* a, const pier_desc_t* b){
    return a->get_auto_group() < b->get_auto_group();
}

koord3d pier_builder_t::lookup_deck_pos(const grund_t *gr, koord3d pos){
    sint8 hdiff;
    if(gr->get_grund_hang()==0){
        hdiff=1;
    }else{
        hdiff=slope_t::max_diff(gr->get_grund_hang());
    }
    return pos+koord3d(0,0,hdiff);
}

const char * pier_builder_t::check_below_ways(player_t *player, koord3d pos, const pier_desc_t *desc, const uint8 rotation, bool halfheight){
    const grund_t *gr = welt->lookup(pos);
    if(gr){
        if(weg_t *w0 = gr->get_weg_nr(0)){
            if( ( w0->get_ribi_unmasked() & desc->get_below_way_ribi(rotation) ) == w0->get_ribi_unmasked()
                    || ( (halfheight) && w0->is_deletable(player) && ( w0->get_waytype()==waytype_t::road_wt
                                                                       || w0->get_waytype()==waytype_t::tram_wt
                                                                       || w0->get_waytype()==waytype_t::water_wt) ) ){
                if(weg_t *w1  = gr->get_weg_nr(1)){
                    if( ( w1->get_ribi_unmasked() & desc->get_below_way_ribi(rotation) ) != w1->get_ribi_unmasked()
                            || ( (halfheight) && w1->is_deletable(player) && ( w1->get_waytype()==waytype_t::road_wt
                                                                               || w1->get_waytype()==waytype_t::tram_wt
                                                                               || w1->get_waytype()==waytype_t::water_wt) ) ){
                        return NULL;
                    }
                }else{
                    return NULL;
                }
            }
        }else{
            return NULL;
        }
    }else{
        return NULL;
    }
    return "Placing a pier here would block way(s)";
}

const char * pier_builder_t::check_for_buildings(const grund_t *gr, const pier_desc_t *desc, const uint8 rotation){
    gebaeude_t *gb = gr->get_building();
    if(gb) {
        if(gb->is_attraction() || gb->is_townhall()){
            return "Cannot build piers on attractions or townhalls";
        }

        //mask allows any buildings
        if(desc->get_sub_obj_mask(rotation)==0xFFFFFFFF){
            return NULL;
        }else if(desc->get_sub_obj_mask(rotation)==0){
            return "Cannot build this type of pier on buildings";
        }else{
            //TODO filter buildings

            return "Cannot build this type of pier on this type of building";
        }
    }

    return NULL;
}

void pier_builder_t::register_desc(pier_desc_t *desc){
    if (const pier_desc_t *old_pier = desc_table.remove(desc->get_name())) {
        dbg->doubled("pier", desc->get_name());
        tool_t::general_tool.remove( old_pier->get_builder() );
        delete old_pier->get_builder();
        delete old_pier;
    }

    //add the tool
    tool_build_pier_t *tool = new tool_build_pier_t;
    tool->set_icon( desc->get_cursor()->get_image_id(1) );
    tool->cursor = desc->get_cursor()->get_image_id(0);
    tool->set_default_param(desc->get_name());
    tool_t::general_tool.append( tool );
    desc->set_builder( tool );
    desc_table.put(desc->get_name(), desc);
}

const pier_desc_t *pier_builder_t::get_desc(const char *name){
    return (name ? desc_table.get(name) : NULL);
}

const char *pier_builder_t::build(player_t *player, koord3d pos, const pier_desc_t *desc, const uint8 rotation){
    grund_t *gr = NULL;

    gr = welt->lookup(pos);
    if(!gr) return "Invalid Ground";

    if(desc->get_background(gr->get_grund_hang(),rotation,0) == IMG_EMPTY){
        return "Invalid ground for this type of pier";
    }

    //check for ways
    const char *msg;
    msg = check_below_ways(player,pos,desc,rotation,false);
    if(msg){
        return msg;
    }
    if(welt->get_settings().get_way_height_clearance()==2){
        msg = check_below_ways(player,pos + koord3d(0,0,-1),desc,rotation,true);
        if(msg){
            return msg;
        }
    }

    //check for buildings
    msg = check_for_buildings(gr,desc,rotation);
    if(msg){
        return msg;
    }

    //check lower pier has enough supporting area
    if(gr->get_typ()==grund_t::pierdeck){
        grund_t *gr2=welt->lookup(gr->get_pos() + koord3d(0,0,-1));
        if(gr2==NULL){
            gr2=welt->lookup(gr->get_pos() + koord3d(0,0,-2));
        }
        uint32 supportmask=pier_t::get_support_mask_total(gr2);
        if((supportmask & desc->get_base_mask(rotation))!=desc->get_base_mask(rotation)){
            return "Cannot build pier.  Lower pier does not provide sufficient footing";
        }
    }

    if(desc->get_middle_mask(rotation) & pier_t::get_middle_mask_total(gr)){
        return "Cannot build pier.  New pier would occupy same space as existing pier";
    }


    //make ground on top (if needed)
    koord3d gpos=lookup_deck_pos(gr, pos);
    if(!welt->lookup(gpos)){
        pier_deck_t *deck = new pier_deck_t(gpos,desc->get_above_slope(),desc->get_above_slope());
        welt->access(pos.get_2d())->boden_hinzufuegen(deck);
        deck->calc_image();
    }

    //create pier
    pier_t *p = new pier_t(pos,player,desc,rotation);
    gr->obj_add(p);

    return NULL;
}

const char *pier_builder_t::remove(player_t *player, koord3d pos){
    grund_t *gr;
    gr = welt->lookup(pos);
    if(!gr) return "Invalid Ground";

    const char *msg=0;
    uint8 pier_cnt=0;
    pier_t *p;
    for(uint8 i = 0; i < gr->get_top(); i++){
        obj_t *ob = gr->obj_bei(i);
        if(ob->get_typ()==obj_t::pier){
            pier_cnt++;
            p=(pier_t*)ob;
        }
    }

    if(pier_cnt==1){
        koord3d gpos=lookup_deck_pos(gr,pos);
        grund_t *bd = welt->lookup(gpos);
        if(bd->obj_count() || bd->get_weg_nr(0)){
            return "Cannot remove sole load bearing pier";
        }
        gr->obj_remove(p);
        delete p;
        welt->access(gpos.get_2d())->boden_entfernen(bd);
        delete bd;
        return NULL;
    }

    for(uint8 j = 0; j < gr->obj_count(); j++){ //try every pier in tile
        obj_t *ob = gr->obj_bei(j);
        if(ob->get_typ()==obj_t::pier){
            pier_cnt++;
            p = (pier_t*)ob;
            if(p->is_deletable(player)==NULL){
                koord3d gpos=lookup_deck_pos(gr,pos);
                grund_t *bd = welt->lookup(gpos);
                //check that we are not supporting another pier
                const char *msg2=NULL;
                for(uint8 i = 0; i < bd->obj_count(); i++){
                    if(bd->obj_bei(i)->get_typ()==obj_t::pier){
                        if(((pier_t*)bd->obj_bei(i))->get_base_mask() & p->get_support_mask()){
                            msg2= "Cannot remove load bearing pier";
                            break;
                        }
                    }
                }
                if(msg2) {
                    if(msg){
                        msg="No piers can be safely removed";
                    }else{
                        msg=msg2;
                    }
                    continue;
                }
                //TODO check that we are not supporting a way
                gr->obj_remove(p);
                delete p;

                return NULL;

            }
        }
    }

    return msg;
}

void pier_builder_t::fill_menu(tool_selector_t *tool_selector){
    if(!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), TOOL_BUILD_PIER | GENERAL_TOOL)){
        return;
    }

    const uint16 time = welt->get_timeline_year_month();
    vector_tpl<const pier_desc_t*> matching(desc_table.get_count());

    for(auto const & i :desc_table){
        pier_desc_t const * const p = i.value;
        if(p->is_available(time)){
            matching.insert_ordered(p, compare_piers);
        }
    }

    FOR(vector_tpl<pier_desc_t const*>, const i, matching){
        tool_selector->add_tool_selector(i->get_builder());
    }

}
