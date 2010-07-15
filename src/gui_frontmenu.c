/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file gui_frontmenu.c
 *     GUI Menus support functions.
 * @par Purpose:
 *     Functions to manage GUI Menus in the game.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     28 May 2010 - 12 Jun 2010
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "gui_frontmenu.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_guibtns.h"
#include "kjm_input.h"
#include "frontend.h"
#include "gui_frontbtns.h"
#include "front_input.h"

#include "keeperfx.hpp"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
DLLIMPORT void _DK_turn_off_roaming_menus(void);
DLLIMPORT void _DK_turn_off_menu(char);
DLLIMPORT void _DK_turn_on_menu(int);//char);
DLLIMPORT void _DK_turn_off_event_box_if_necessary(long plridx, char val);
DLLIMPORT unsigned long _DK_turn_off_all_window_menus(void);
DLLIMPORT void _DK_turn_off_all_panel_menus(void);

/******************************************************************************/
struct GuiMenu *get_active_menu(MenuNumber num)
{
    if (num < 0)
        num = 0;
    if (num >= ACTIVE_MENUS_COUNT)
        num = 0;
    return &active_menus[num];
}

int first_monopoly_menu(void)
{
  int idx;
  struct GuiMenu *gmnu;
  for (idx=0;idx<ACTIVE_MENUS_COUNT;idx++)
  {
    gmnu=&active_menus[idx];
    if ((gmnu->field_1!=0) && (gmnu->flgfield_1E!=0))
        return idx;
  }
  return -1;
}

MenuNumber menu_id_to_number(MenuID menu_id)
{
    MenuNumber idx;
    struct GuiMenu *gmnu;
    for(idx=0; idx < ACTIVE_MENUS_COUNT; idx++)
    {
      gmnu = &active_menus[idx];
      if ((gmnu->field_1 != 0) && (gmnu->field_0 == menu_id))
        return idx;
    }
    return MENU_INVALID_ID;
}

/**
 * Checks if the given screen point is over a gui menu.
 * @param x,y Screen coordinates to check.
 * @return Returns index of the menu, or -1 if there's no menu on this point.
 */
int point_is_over_gui_menu(long x, long y)
{
  int idx;
  int gidx = MENU_INVALID_ID;
  for(idx=0; idx < ACTIVE_MENUS_COUNT; idx++)
  {
    struct GuiMenu *gmnu;
    gmnu=&active_menus[idx];
    if (gmnu->field_1 != 2)
        continue;
    if (gmnu->flgfield_1D == 0)
        continue;
    short gx = gmnu->pos_x;
    if ((x >= gx) && (x < gx+gmnu->width))
    {
        short gy = gmnu->pos_y;
        if ((y >= gy) && (y < gy+gmnu->height))
            gidx=idx;
    }
  }
  return gidx;
}

void update_busy_doing_gui_on_menu(void)
{
  int gidx;
  gidx = point_is_over_gui_menu(GetMouseX(), GetMouseY());
  if (gidx == -1)
    busy_doing_gui = 0;
  else
    busy_doing_gui = 1;
}

void turn_off_menu(MenuID mnu_idx)
{
  struct GuiMenu *gmnu;
  long menu_num;
  if ((mnu_idx >= 13) && (mnu_idx <= 14))
    save_settings();
  menu_num = menu_id_to_number(mnu_idx);
  if (menu_num >= 0)
  {
    if (game_is_busy_doing_gui_string_input())
    {
      if (input_button->gmenu_idx == menu_num)
        kill_button_area_input();
    }
    gmnu = get_active_menu(menu_num);
    gmnu->field_1 = 3;
    if (update_menu_fade_level(gmnu) == -1)
    {
      kill_menu(gmnu);
      remove_from_menu_stack(gmnu->field_0);
    }
  }
}

void turn_off_roaming_menus(void)
{
  _DK_turn_off_roaming_menus();
}

void turn_off_query_menus(void)
{
  turn_off_menu(31);
  turn_off_menu(35);
  turn_off_menu(32);
}

void turn_off_all_panel_menus(void)
{
  int mnu_num;
  struct GuiMenu *gmnu;
  mnu_num = menu_id_to_number(1);
  if (mnu_num >= 0)
  {
    gmnu = get_active_menu(mnu_num);
    setup_radio_buttons(gmnu);
  }
  if ( menu_is_active(2) )
  {
    turn_off_menu(2);
  }
  if ( menu_is_active(3) )
  {
    turn_off_menu(3);
  }
  if ( menu_is_active(4) )
  {
    turn_off_menu(4);
  }
  if ( menu_is_active(7) )
  {
    turn_off_menu(7);
  }
  if ( menu_is_active(5) )
  {
    turn_off_menu(5);
  }
  if ( menu_is_active(31) )
  {
    turn_off_menu(31);
  }
  if ( menu_is_active(35) )
  {
    turn_off_menu(35);
  }
  if ( menu_is_active(32) )
  {
    turn_off_menu(32);
  }
  if ( menu_is_active(38) )
  {
    turn_off_menu(38);
  }
}

void set_menu_mode(long mnu_idx)
{
  if (!menu_is_active(mnu_idx))
  {
    turn_off_all_panel_menus();
    turn_on_menu(mnu_idx);
  }
}

short turn_off_all_window_menus(void)
{
  short result;
  result = false;
  if (menu_is_active(10))
  {
    result = true;
    turn_off_menu(10);
  }
  if (menu_is_active(11))
  {
    result = true;
    set_packet_pause_toggle();
    turn_off_menu(11);
  }
  if (menu_is_active(GMnu_SAVE))
  {
    result = true;
    set_packet_pause_toggle();
    turn_off_menu(GMnu_SAVE);
  }
  if (menu_is_active(GMnu_OPTIONS))
  {
    result = true;
    turn_off_menu(GMnu_OPTIONS);
  }
  if (menu_is_active(GMnu_VIDEO))
  {
    result = true;
    turn_off_menu(GMnu_VIDEO);
  }
  if (menu_is_active(GMnu_SOUND))
  {
    result = true;
    turn_off_menu(GMnu_SOUND);
  }
  if (menu_is_active(GMnu_ERROR_BOX))
  {
    result = true;
    turn_off_menu(GMnu_ERROR_BOX);
  }
  if (menu_is_active(GMnu_INSTANCE))
  {
    result = true;
    turn_off_menu(GMnu_INSTANCE);
  }
  if (menu_is_active(GMnu_RESURRECT_CREATURE))
  {
    result = true;
    turn_off_menu(GMnu_RESURRECT_CREATURE);
  }
  if (menu_is_active(GMnu_TRANSFER_CREATURE))
  {
    result = true;
    turn_off_menu(GMnu_TRANSFER_CREATURE);
  }
  if (menu_is_active(GMnu_ARMAGEDDON))
  {
    result = true;
    turn_off_menu(GMnu_ARMAGEDDON);
  }
  if (menu_is_active(GMnu_AUTOPILOT))
  {
    result = true;
    turn_off_menu(GMnu_AUTOPILOT);
  }
  if (menu_is_active(GMnu_SPELL_LOST))
  {
    result = true;
    turn_off_menu(GMnu_SPELL_LOST);
  }
  return result;
}

void turn_on_main_panel_menu(void)
{
  if (menu_id_to_number(GMnu_MAIN) == -1)
  {
    turn_on_menu(GMnu_MAIN);
  }
  if (info_tag != 0)
  {
    turn_on_menu(GMnu_QUERY);
  } else
  if (room_tag != 0)
  {
    turn_on_menu(GMnu_ROOM);
  } else
  if (spell_tag != 0)
  {
    turn_on_menu(GMnu_SPELL);
  } else
  if (trap_tag != 0)
  {
    turn_on_menu(GMnu_TRAP);
  } else
  if (creature_tag != 0)
  {
    turn_on_menu(GMnu_CREATURE);
  }
}

short turn_off_all_bottom_menus(void)
{
  short result;
  result = false;
  if (menu_is_active(16))
  {
    result = true;
    turn_off_menu(16);
  }
  if (menu_is_active(34))
  {
    result = true;
    turn_off_menu(34);
  }
  if (menu_is_active(27))
  {
    result = true;
    turn_off_menu(27);
  }
  return result;
}

void turn_off_all_menus(void)
{
  turn_off_all_panel_menus();
  turn_off_all_window_menus();
  turn_off_all_bottom_menus();
}

void turn_on_menu(MenuID idx)
{
    struct GuiMenu *gmnu = NULL;
    gmnu = menu_list[idx];
    if (create_menu(gmnu))
    {
      if (gmnu->field_1F)
        game.field_1517F6 = idx;
    }
}

void set_menu_visible_on(MenuID menu_id)
{
  long menu_num;
  menu_num = menu_id_to_number(menu_id);
  if (menu_num < 0)
    return;
  get_active_menu(menu_num)->flgfield_1D = 1;
  int idx;
  for (idx=0; idx<ACTIVE_BUTTONS_COUNT; idx++)
  {
    struct GuiButton *gbtn = &active_buttons[idx];
    if (gbtn->field_0 & 1)
    {
      Gf_Btn_Callback callback;
      callback = gbtn->field_17;
      if ((gbtn->gmenu_idx == menu_num) && (callback != NULL))
        callback(gbtn);
    }
  }
}

void set_menu_visible_off(MenuID menu_id)
{
    MenuNumber menu_num;
    menu_num = menu_id_to_number(menu_id);
    if (menu_num < 0)
      return;
    get_active_menu(menu_num)->flgfield_1D = 0;
}

void kill_menu(struct GuiMenu *gmnu)
{
  struct GuiButton *gbtn;
  int i;
  if (gmnu->field_1)
  {
    gmnu->field_1 = 0;
    for (i=0; i<ACTIVE_BUTTONS_COUNT; i++)
    {
      gbtn = &active_buttons[i];
      if ((gbtn->field_0 & 0x01) && (gbtn->gmenu_idx == gmnu->field_14))
        kill_button(gbtn);
    }
  }
}

void remove_from_menu_stack(short mnu_id)
{
  unsigned short i;
  for (i=0; i<no_of_active_menus; i++)
  {
    if (menu_stack[i] == mnu_id)
    {
      while (i < no_of_active_menus-1)
      {
        menu_stack[i] = menu_stack[i+1];
        i++;
      }
      break;
    }
  }
  if (i < no_of_active_menus)
    no_of_active_menus--;
}

void add_to_menu_stack(unsigned char mnu_idx)
{
  short i;
  if (no_of_active_menus >= ACTIVE_MENUS_COUNT)
  {
    ERRORLOG("No more room on menu stack");
    return;
  }

  for (i=0; i<no_of_active_menus; i++)
  {
    if (menu_stack[i] == mnu_idx)
    { // If already in stack, move it at end of the stack.
      while (i < no_of_active_menus-1)
      {
        menu_stack[i] = menu_stack[i+1];
        i++;
      }
      menu_stack[(int)no_of_active_menus-1] = mnu_idx;
      //SYNCMSG("Menu %d moved to end of stack, at position %d.",mnu_idx,no_of_active_menus-1);
      return;
    }
  }
  // If not in stack, add at end
  menu_stack[(unsigned char)no_of_active_menus] = mnu_idx;
  no_of_active_menus++;
  SYNCDBG(9,"Menu %d put on stack, at position %d.",mnu_idx,no_of_active_menus-1);
}

long first_available_menu(void)
{
  short i;
  for (i=0; i<ACTIVE_MENUS_COUNT; i++)
  {
    if (active_menus[i].field_1 == 0)
      return i;
  }
  return -1;
}

void turn_off_event_box_if_necessary(long plridx, char val)
{
  _DK_turn_off_event_box_if_necessary(plridx, val);
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif
