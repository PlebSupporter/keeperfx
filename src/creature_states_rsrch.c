/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file creature_states_rsrch.c
 *     Creature state machine functions for their job in various rooms.
 * @par Purpose:
 *     Defines elements of states[] array, containing valid creature states.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     23 Sep 2009 - 05 Jan 2011
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "creature_states_rsrch.h"
#include "globals.h"

#include "bflib_math.h"
#include "creature_states.h"
#include "creature_instances.h"
#include "thing_list.h"
#include "creature_control.h"
#include "creature_jobs.h"
#include "creature_states_combt.h"
#include "config_creature.h"
#include "config_rules.h"
#include "config_terrain.h"
#include "thing_stats.h"
#include "thing_objects.h"
#include "thing_effects.h"
#include "thing_navigate.h"
#include "room_data.h"
#include "room_jobs.h"
#include "gui_soundmsgs.h"
#include "game_legacy.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
DLLIMPORT short _DK_at_research_room(struct Thing *creatng);
DLLIMPORT long _DK_process_research_function(struct Thing *creatng);
DLLIMPORT short _DK_researching(struct Thing *creatng);
DLLIMPORT void _DK_force_complete_current_research(long plyr_idx);
DLLIMPORT long _DK_get_next_research_item(struct Dungeon *dungeon);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
/******************************************************************************/
TbBool creature_can_do_research(const struct Thing *creatng)
{
    if (is_neutral_thing(creatng)) {
        return false;
    }
    struct CreatureStats *crstat;
    struct Dungeon *dungeon;
    crstat = creature_stats_get_from_thing(creatng);
    dungeon = get_dungeon(creatng->owner);
    return (crstat->research_value > 0) && (dungeon->current_research_idx >= 0);
}

short at_research_room(struct Thing *thing)
{
    struct CreatureControl *cctrl;
    struct Dungeon *dungeon;
    struct Room *room;
    //return _DK_at_research_room(thing);
    cctrl = creature_control_get_from_thing(thing);
    cctrl->target_room_id = 0;
    dungeon = get_dungeon(thing->owner);
    if (!creature_can_do_research(thing))
    {
        if (!is_neutral_thing(thing) && (dungeon->current_research_idx < 0))
        {
            if (is_my_player_number(dungeon->owner))
                output_message(SMsg_NoMoreReseach, 500, true);
        }
        set_start_state(thing);
        return 0;
    }
    room = get_room_thing_is_on(thing);
    if (!room_initially_valid_as_type_for_thing(room, RoK_LIBRARY, thing))
    {
        WARNLOG("Room %s owned by player %d is invalid for %s",room_code_name(room->kind),(int)room->owner,thing_model_name(thing));
        set_start_state(thing);
        return 0;
    }
    if (!add_creature_to_work_room(thing, room))
    {
        set_start_state(thing);
        return 0;
    }
    if ( !setup_random_head_for_room(thing, room, 0) )
    {
        ERRORLOG("The %s can not move in research room", thing_model_name(thing));
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return 0;
    }
    thing->continue_state = CrSt_Researching;
    cctrl->field_82 = 0;
    cctrl->byte_9A = 3;
    return 1;
}

TbBool research_needed(const struct ResearchVal *rsrchval, const struct Dungeon *dungeon)
{
    if (dungeon->research_num == 0)
        return false;
    switch (rsrchval->rtyp)
    {
   case RsCat_Power:
        if ( (dungeon->magic_resrchable[rsrchval->rkind]) && (dungeon->magic_level[rsrchval->rkind] == 0) )
        {
            return true;
        }
        break;
    case RsCat_Room:
        if ( (dungeon->room_resrchable[rsrchval->rkind]) && (dungeon->room_buildable[rsrchval->rkind] == 0) )
        {
            return true;
        }
        break;
    case RsCat_Creature:
        if ((dungeon->creature_allowed[rsrchval->rkind]) && (dungeon->creature_force_enabled[rsrchval->rkind] == 0))
        {
            return true;
        }
        break;
    case RsCat_None:
        break;
    default:
        ERRORLOG("Illegal research type %d while processing player research",(int)rsrchval->rtyp);
        break;
    }
    return false;
}

/**
 * Returns new research item index, or -1 if nothing to research.
 * @param dungeon
 * @return
 */
int get_next_research_item(const struct Dungeon *dungeon)
{
    const struct ResearchVal *rsrchval;
    long resnum;
    //return _DK_get_next_research_item(dungeon);
    if (dungeon->research_num == 0)
        return -1;
    for (resnum = 0; resnum < dungeon->research_num; resnum++)
    {
        rsrchval = &dungeon->research[resnum];
        switch (rsrchval->rtyp)
        {
       case RsCat_Power:
            if ( (dungeon->magic_resrchable[rsrchval->rkind]) && (dungeon->magic_level[rsrchval->rkind] == 0) )
            {
                return resnum;
            }
            break;
        case RsCat_Room:
            if ( (dungeon->room_resrchable[rsrchval->rkind]) && (dungeon->room_buildable[rsrchval->rkind] == 0) )
            {
                return resnum;
            }
            break;
        case RsCat_Creature:
            WARNLOG("Creature research skipped - not implemented");
            break;
        case RsCat_None:
            break;
        default:
            ERRORLOG("Illegal research type %d while getting next research item",(int)rsrchval->rtyp);
            break;
        }
    }
    return -1;
}

struct ResearchVal *get_players_current_research_val(PlayerNumber plyr_idx)
{
    struct Dungeon *dungeon;
    dungeon = get_dungeon(plyr_idx);
    if ((dungeon->current_research_idx < 0) || (dungeon->current_research_idx >= DUNGEON_RESEARCH_COUNT))
        return NULL;
    return &dungeon->research[dungeon->current_research_idx];
}

TbBool force_complete_current_research(PlayerNumber plyr_idx)
{
    struct Dungeon *dungeon;
    struct ResearchVal *rsrchval;
    //_DK_force_complete_current_research(plyr_idx);
    dungeon = get_dungeon(plyr_idx);
    rsrchval = get_players_current_research_val(plyr_idx);
    if (rsrchval != NULL)
    {
        if ( research_needed(rsrchval, dungeon) ) {
            dungeon->field_1193 = rsrchval->req_amount << 8;
            return true;
        }
    }
    dungeon->current_research_idx = get_next_research_item(dungeon);
    rsrchval = get_players_current_research_val(plyr_idx);
    if (rsrchval != NULL)
    {
        dungeon->field_1193 = rsrchval->req_amount << 8;
        return true;
    }
    return false;
}

#define INSTANCE_RET_IF_AVAIL_AND_RESET(thing, inst_id) \
    if (creature_instance_is_available(thing, inst_id) \
      && creature_instance_has_reset(thing, inst_id)) { \
        return inst_id; \
    }
#define INSTANCE_RET_NEG_IF_AVAIL_ONLY(thing, inst_id) \
    if (creature_instance_is_available(thing, inst_id)) { \
        return -inst_id; \
    }
long get_best_quick_range_instance_to_use(struct Thing *thing)
{
    INSTANCE_RET_IF_AVAIL_AND_RESET(thing, CrInst_FIREBALL);
    INSTANCE_RET_IF_AVAIL_AND_RESET(thing, CrInst_FIRE_ARROW);
    INSTANCE_RET_IF_AVAIL_AND_RESET(thing, CrInst_MISSILE);
    INSTANCE_RET_IF_AVAIL_AND_RESET(thing, CrInst_NAVIGATING_MISSILE);
    INSTANCE_RET_IF_AVAIL_AND_RESET(thing, CrInst_LIGHTNING);
    INSTANCE_RET_IF_AVAIL_AND_RESET(thing, CrInst_HAILSTORM);
    INSTANCE_RET_IF_AVAIL_AND_RESET(thing, CrInst_GRENADE);
    INSTANCE_RET_IF_AVAIL_AND_RESET(thing, CrInst_POISON_CLOUD);
    INSTANCE_RET_NEG_IF_AVAIL_ONLY(thing, CrInst_FIREBALL);
    INSTANCE_RET_NEG_IF_AVAIL_ONLY(thing, CrInst_FIRE_ARROW);
    INSTANCE_RET_NEG_IF_AVAIL_ONLY(thing, CrInst_MISSILE);
    INSTANCE_RET_NEG_IF_AVAIL_ONLY(thing, CrInst_NAVIGATING_MISSILE);
    INSTANCE_RET_NEG_IF_AVAIL_ONLY(thing, CrInst_LIGHTNING);
    INSTANCE_RET_NEG_IF_AVAIL_ONLY(thing, CrInst_HAILSTORM);
    INSTANCE_RET_NEG_IF_AVAIL_ONLY(thing, CrInst_GRENADE);
    INSTANCE_RET_NEG_IF_AVAIL_ONLY(thing, CrInst_POISON_CLOUD);
    return CrInst_NULL;
}
#undef INSTANCE_RET_IF_AVAIL_AND_RESET
#undef INSTANCE_RET_NEG_IF_AVAIL_ONLY

TbBool find_combat_target_passing_by_subtile_but_having_unrelated_job(const struct Thing *creatng, CreatureJob job_kind, MapSubtlCoord stl_x, MapSubtlCoord stl_y, unsigned long *found_dist, struct Thing **found_thing)
{
    struct Thing *thing;
    struct Map *mapblk;
    long i;
    unsigned long k;
    long dist;
    mapblk = get_map_block_at(stl_x,stl_y);
    k = 0;
    i = get_mapwho_thing_index(mapblk);
    while (i != 0)
    {
        thing = thing_get(i);
        TRACE_THING(thing);
        if (thing_is_invalid(thing))
        {
            ERRORLOG("Jump to invalid thing detected");
            break;
        }
        i = thing->next_on_mapblk;
        // Per thing code start
        if (thing_is_creature(thing) && (thing->index != creatng->index) && !creature_has_job(thing, job_kind)
            && !creature_is_kept_in_custody(thing) && !creature_is_being_unconscious(thing)
            && !creature_is_dying(thing) && !creature_is_doing_anger_job(thing))
        {
            if (!creature_is_invisible(thing) || creature_can_see_invisible(creatng))
            {
                dist = get_combat_distance(creatng, thing);
                // If we have combat sight - we want that target, don't search anymore
                if ( creature_can_see_combat_path(creatng, thing, dist) > 0 )
                {
                    *found_dist = dist;
                    *found_thing = thing;
                    return true;
                }
                // No combat sight - but maybe it's at least closer than previous one
                if ( *found_dist > dist )
                {
                    *found_dist = dist;
                    *found_thing = thing;
                }
            }
        }
        // Per thing code end
        k++;
        if (k > THINGS_COUNT)
        {
            ERRORLOG("Infinite loop detected when sweeping things list");
            break;
        }
    }
    return false;
}

/**
 * Finds a creature passing by a subtile and having job different than given one, which has direct combat sight for given creature.
 * If such creature is not found, the function returns false and gives closest creature passing by a subtile and having job different than given one.
 * @param creatng The creature which should have combat sight of the target.
 * @param job_kind The job target should'n be performing.
 * @param slb_x Subtile on which target creature is searched, X coord.
 * @param slb_y Subtile on which target creature is searched, Y coord.
 * @param found_dist Distance to the target creature found.
 * @param found_thing The target creature found, either closest one or one with combat sight.
 * @return True if a target with combat sight was found. False if closest creature was found, or no creature met the conditions.
 * @note If no creature met the conditions, output variables are not initialized. Therefore, they should be initialized before calling this function.
 */
TbBool find_combat_target_passing_by_slab_but_having_unrelated_job(const struct Thing *creatng, CreatureJob job_kind, MapSlabCoord slb_x, MapSlabCoord slb_y, unsigned long *found_dist, struct Thing **found_thing)
{
    MapSubtlCoord endstl_x,endstl_y;
    MapSubtlCoord stl_x,stl_y;
    endstl_x = 3*slb_x+3;
    endstl_y = 3*slb_y+3;
    for (stl_y = 3*slb_y; stl_y < endstl_y; stl_y++)
    {
        for (stl_x = 3*slb_x; stl_x < endstl_x; stl_x++)
        {
            if (find_combat_target_passing_by_subtile_but_having_unrelated_job(creatng, job_kind, stl_x, stl_y, found_dist, found_thing))
                return true;
        }
    }
    // If found a creature, but it's not on sight
    return false;
}

/**
 * Finds a creature passing by a room and having job different than given one, which has direct combat sight for given creature.
 * If such creature is not found, the function returns false and gives closest creature passing by a room and having job different than given one.
 * @param creatng The creature which should have combat sight of the target.
 * @param job_kind The job target should'n be performing.
 * @param room The room on which target creature is searched.
 * @param found_dist Distance to the target creature found.
 * @param found_thing The target creature found, either closest one or one with combat sight.
 * @return True if a target with combat sight was found. False if closest creature was found, or no creature met the conditions.
 * @note If no creature met the conditions, output variables are not initialized. Therefore, they should be initialized before calling this function.
 */
TbBool find_combat_target_passing_by_room_but_having_unrelated_job(const struct Thing *creatng, CreatureJob job_kind, const struct Room *room, unsigned long *found_dist, struct Thing **found_thing)
{
    unsigned long i;
    unsigned long k;
    k = 0;
    i = room->slabs_list;
    while (i > 0)
    {
        MapSubtlCoord slb_x,slb_y;
        slb_x = slb_num_decode_x(i);
        slb_y = slb_num_decode_y(i);
        // Per-slab code
        if (find_combat_target_passing_by_slab_but_having_unrelated_job(creatng, job_kind, slb_x, slb_y, found_dist, found_thing)) {
            return true;
        }
        // Per-slab code ends
        i = get_next_slab_number_in_room(i);
        k++;
        if (k > room->slabs_count)
        {
            ERRORLOG("Infinite loop detected when sweeping room slabs");
            break;
        }
    }
    // If found a creature, but it's not on sight
    return false;
}

/**
 * Processes job stress.
 * Creatures which aren't performing their primary jobs can be attacked by creatures
 * affected by job stress.
 * Job stress causes creature working in room to attack other creatures walking
 * through the same room which aren't performing the same job (ie. are just passing by),
 * or other workers in that room who does not have the job as primary job.
 * @param creatng The thing being affected by job stress.
 * @param room The room where target creature should be searched for.
 * @return
 */
TbBool process_job_stress(struct Thing *creatng, struct Room *room)
{
    struct CreatureControl *cctrl;
    struct CreatureStats *crstat;
    CreatureJob stressful_job;
    cctrl = creature_control_get_from_thing(creatng);
    crstat = creature_stats_get_from_thing(creatng);
    if ( (crstat->job_stress <= 0) || (cctrl->instance_id != CrInst_NULL) ) {
        return false;
    }
    // Process the stress once per 20 turns
    if (((game.play_gameturn + creatng->index) % 20) != 0) {
        return false;
    }
    // Make sure we really should react on job stress
    stressful_job = get_creature_job_causing_stress(crstat->job_primary,room->kind);
    if (stressful_job == Job_NULL) {
        return false;
    }
    long inst_use;
    inst_use = get_best_quick_range_instance_to_use(creatng);
    if (inst_use <= 0) {
        return false;
    }
    // Find a target
    unsigned long combt_dist;
    struct Thing *combt_thing;
    combt_dist = LONG_MAX;
    combt_thing = INVALID_THING;
    if (find_combat_target_passing_by_room_but_having_unrelated_job(creatng, stressful_job, room, &combt_dist, &combt_thing))
    {
        struct CreatureControl *combctrl;
        set_creature_instance(creatng, inst_use, 0, combt_thing->index, 0);
        external_set_thing_state(combt_thing, CrSt_CreatureEvacuateRoom);
        combctrl = creature_control_get_from_thing(combt_thing);
        combctrl->word_9A = room->index;
        anger_apply_anger_to_creature(creatng, crstat->annoy_job_stress, AngR_Other, 1);
        return true;
    }
    if (thing_is_invalid(combt_thing)) {
        return false;
    }
    if ( !setup_person_move_to_position(creatng, combt_thing->mappos.x.stl.num, combt_thing->mappos.y.stl.num, 0) ) {
        return false;
    }
    creatng->continue_state = CrSt_Researching;
    cctrl->field_82 = 0;
    cctrl->byte_9A = 3;
    return true;
}

/**
 * Does a step of researching.
 * Informs if the research cycle should end.
 * @param thing
 */
CrCheckRet process_research_function(struct Thing *thing)
{
    struct Dungeon *dungeon;
    struct Room *room;
    //return _DK_process_research_function(thing);
    dungeon = get_dungeon(thing->owner);
    if (dungeon_invalid(dungeon)) {
        SYNCDBG(9,"The %s index %d cannot work as player %d has no dungeon",
            thing_model_name(thing), (int)thing->index, (int)thing->owner);
        set_start_state(thing);
        return CrCkRet_Continue;
    }
    if (!creature_can_do_research(thing)) {
        set_start_state(thing);
        return CrCkRet_Continue;
    }
    room = get_room_creature_works_in(thing);
    if ( !room_still_valid_as_type_for_thing(room, RoK_LIBRARY, thing) ) {
        WARNLOG("Room %s owned by player %d is bad work place for %s owned by played %d",
            room_code_name(room->kind), (int)room->owner, thing_model_name(thing), (int)thing->owner);
        set_start_state(thing);
        return CrCkRet_Continue;
    }
    long work_value;
    struct CreatureControl *cctrl;
    cctrl = creature_control_get_from_thing(thing);
    struct CreatureStats *crstat;
    crstat = creature_stats_get_from_thing(thing);
    work_value = compute_creature_work_value(crstat->research_value*256, room->efficiency, cctrl->explevel);
    work_value = process_work_speed_on_work_value(thing, work_value);
    SYNCDBG(19,"The %s index %d produced %d research points",thing_model_name(thing),(int)thing->index,(int)work_value);
    dungeon->total_research_points += work_value;
    dungeon->field_1193 += work_value;
    process_job_stress(thing, room);
    return CrCkRet_Available;
}

short researching(struct Thing *thing)
{
    struct Dungeon *dungeon;
    long i;
    TRACE_THING(thing);
    //return _DK_researching(thing);
    dungeon = get_dungeon(thing->owner);
    if (is_neutral_thing(thing))
    {
        ERRORLOG("Neutral %s can't do research",thing_model_name(thing));
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_Unchanged;
    }
    if (!creature_can_do_research(thing))
    {
        if (!is_neutral_thing(thing) && (dungeon->current_research_idx < 0))
        {
            if (is_my_player_number(dungeon->owner))
                output_message(SMsg_NoMoreReseach, 500, true);
        }
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_Unchanged;
    }
    // Get and verify working room
    struct Room *room;
    room = get_room_thing_is_on(thing);
    if (creature_work_in_room_no_longer_possible(room, RoK_LIBRARY, thing))
    {
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_ResetFail;
    }

    if (room->used_capacity > room->total_capacity)
    {
        if (is_my_player_number(room->owner))
            output_message(SMsg_LibraryTooSmall, 0, true);
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_Unchanged;
    }
    process_research_function(thing);
    struct CreatureControl *cctrl;
    cctrl = creature_control_get_from_thing(thing);
    if ( (game.play_gameturn - dungeon->field_AE5 < 50)
      && ((game.play_gameturn + thing->index) & 0x03) == 0)
    {
        external_set_thing_state(thing, CrSt_CreatureBeHappy);
        cctrl->field_282 = 50;
        cctrl->long_9A = 0;
        return CrStRet_Modified;
    }
    if (cctrl->instance_id != CrInst_NULL)
      return 1;
    cctrl->field_82++;
    // Shall we do some "Standing and thinking"
    if (cctrl->field_82 <= 128)
    {
      if (cctrl->byte_9A == 3)
      {
          // Do some random thinking
          if ((cctrl->field_82 % 16) == 0)
          {
              i = ACTION_RANDOM(1024) - 512;
              cctrl->long_9B = ((long)thing->field_52 + i) & 0x7FF;
              cctrl->byte_9A = 4;
          }
      } else
      {
          // Look at different direction while thinking
          if ( creature_turn_to_face_angle(thing, cctrl->long_9B) < 56 )
          {
              cctrl->byte_9A = 3;
          }
      }
      return 1;
    }
    // Finished "Standing and thinking" - make "new idea" effect and go to next position
    if ( !setup_random_head_for_room(thing, room, 0) )
    {
        ERRORLOG("Cannot move %s in %s room", thing_model_name(thing),room_code_name(room->kind));
        set_start_state(thing);
        return 1;
    }
    thing->continue_state = CrSt_Researching;
    cctrl->field_82 = 0;
    cctrl->byte_9A = 3;
    if (cctrl->explevel < 3)
    {
        create_effect(&thing->mappos, TngEff_Unknown54, thing->owner);
    } else
    if (cctrl->explevel < 6)
    {
        create_effect(&thing->mappos, TngEff_Unknown55, thing->owner);
    } else
    {
        create_effect(&thing->mappos, TngEff_Unknown56, thing->owner);
    }
    return 1;
}

/******************************************************************************/
