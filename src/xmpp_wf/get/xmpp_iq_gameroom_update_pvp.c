/**
 * WarfaceBot, a blind XMPP client for Warface (FPS)
 * Copyright (C) 2015-2017 Levak Borok <levak92@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <wb_tools.h>
#include <wb_session.h>
#include <wb_xmpp.h>
#include <wb_xmpp_wf.h>
#include <wb_log.h>
#include <wb_status.h>
#include <wb_lang.h>

#include <stdlib.h>

struct cb_args
{
    f_gameroom_update_pvp_cb cb;
    void *args;
};

static void xmpp_iq_gameroom_update_pvp_cb(const char *msg,
                                          enum xmpp_msg_type type,
                                          void *args)
{
    /* Answer :
       <iq to='masterserver@warface/pve_2' type='get'>
        <query xmlns='urn:cryonline:k01'>
         <data query_name='gameroom_update_pvp' compressedData='...'
               originalSize='42'/>
        </query>
       </iq>
     */

    struct cb_args *a = (struct cb_args *) args;

    if (type & XMPP_TYPE_ERROR)
    {
        const char *reason = NULL;

        int code = get_info_int(msg, "code='", "'", NULL);
        int custom_code = get_info_int(msg, "custom_code='", "'", NULL);

        switch (code)
        {
            case 8:
                switch (custom_code)
                {
                    case 1:
                        reason = LANG(error_unknown_mission);
                        break;
                    case 7:
                        reason = LANG(error_room_started);
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }

        if (reason != NULL)
            eprintf("%s (%s)",
                    LANG(error_gameroom_setinfo),
                    reason);
        else
            eprintf("%s (%i:%i)",
                    LANG(error_gameroom_setinfo),
                    code,
                    custom_code);
    }
    else
    {
        char *data = wf_get_query_content(msg);

        if (data == NULL)
            return;

        gameroom_sync(data);

        status_set(session.online.status);

        if (a->cb)
            a->cb(a->args);

        free(data);
    }

    free(a);
}

void xmpp_iq_gameroom_update_pvp(const char *mission_key,
                                 enum pvp_mode flags,
                                 int max_players,
                                 int inventory_slot,
                                 f_gameroom_update_pvp_cb cb,
                                 void *args)
{
    if (mission_key == NULL)
        return;

    struct cb_args *a = calloc(1, sizeof (struct cb_args));

    a->cb = cb;
    a->args = args;

    xmpp_send_iq_get(
        JID_MS(session.online.channel),
        xmpp_iq_gameroom_update_pvp_cb, a,
        "<query xmlns='urn:cryonline:k01'>"
        " <gameroom_update_pvp by_mission_key='1' mission_key='%s'"
        "    private='%d'"
        "    friendly_fire='%d'"
        "    enemy_outlines='%d'"
        "    auto_team_balance='%d'"
        "    dead_can_chat='%d'"
        "    join_in_the_process='%d'"
        "    max_players='%d' inventory_slot='%d'>"
        "  <class_rifleman enabled='1' class_id='0'/>"
        "  <class_engineer enabled='1' class_id='4'/>"
        "  <class_medic enabled='1' class_id='3'/>"
        "  <class_sniper enabled='1' class_id='2'/>"
        "  <class_heavy enabled='1' class_id='1'/>"
        " </gameroom_update_pvp>"
        "</query>",
        mission_key,
        flags & PVP_PRIVATE ? 1 : 0,
        flags & PVP_FRIENDLY_FIRE ? 1 : 0,
        flags & PVP_ENEMY_OUTLINES ? 1 : 0,
        flags & PVP_AUTOBALANCE ? 1 : 0,
        flags & PVP_DEADCHAT ? 1 : 0,
        flags & PVP_ALLOWJOIN ? 1 : 0,
        max_players,
        inventory_slot);
}
