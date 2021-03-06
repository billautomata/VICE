/*
 * drive-resources.c - Hardware-level disk drive emulation, resource module.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "drive-check.h"
#include "drive-resources.h"
#include "drive.h"
#include "drivecpu.h"
#include "driverom.h"
#include "drivetypes.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "lib.h"
#include "log.h"
#include "machine-bus.h"
#include "machine-drive.h"
#include "resources.h"
#include "vdrive-bam.h"


/* Is true drive emulation switched on?  */
static int drive_true_emulation;

static int drive1_resources_type(int val, void *param);


static int set_drive_true_emulation(int val, void *param)
{
    unsigned int dnr;
    drive_t *drive;

    drive_true_emulation = val;

    machine_bus_status_truedrive_set((unsigned int)drive_true_emulation);

    if (val) {
        for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
            drive = drive_context[dnr]->drive;
            if (drive->type != DRIVE_TYPE_NONE) {
                drive->enable = 1;
                drivecpu_reset_clk(drive_context[dnr]);
            }
        }
        for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
            drive_enable(drive_context[dnr]);
        }
    } else {
        for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
            drive = drive_context[dnr]->drive;
            drive_disable(drive_context[dnr]);
            if (drive->image != NULL) {
                vdrive_bam_reread_bam(dnr + 8);
            }
        }
    }
    return 0;
}

static int set_drive_extend_image_policy(int val, void *param)
{
    switch (val) {
      case DRIVE_EXTEND_NEVER:
      case DRIVE_EXTEND_ASK:
      case DRIVE_EXTEND_ACCESS:
        drive_context[(int)param]->drive->extend_image_policy = val;
        return 0;
      default:
        return -1;
    }
}

static int drive0_resources_type(int val, void *param)
{
    unsigned int type;
    int busses;
    drive_t *drive;

    drive = drive_context[0]->drive;
    type = (unsigned int)val;
    busses = iec_available_busses();

    /* if bus for drive type is not allowed, set to default value for bus */
    if (!drive_check_bus(type, 0, busses)) {
        if (busses & IEC_BUS_IEC) {
            type = DRIVE_TYPE_1541;
        } else
        if (busses & IEC_BUS_IEEE) {
            type = DRIVE_TYPE_2031;
        } else
            type = DRIVE_TYPE_NONE;
    }

    if (drive_check_dual(type)) {
        /* dual disk drives disable second emulated unit */
        log_warning(drive->log,
                    "Dual disk drive disables second emulated drive");

        drive1_resources_type(DRIVE_TYPE_NONE, NULL);
    }

    switch (type) {
      case DRIVE_TYPE_1541:
      case DRIVE_TYPE_1541II:
      case DRIVE_TYPE_1551:
      case DRIVE_TYPE_1570:
      case DRIVE_TYPE_1571:
      case DRIVE_TYPE_1571CR:
      case DRIVE_TYPE_1581:
      case DRIVE_TYPE_2031:
      case DRIVE_TYPE_1001:
      case DRIVE_TYPE_2040:
      case DRIVE_TYPE_3040:
      case DRIVE_TYPE_4040:
      case DRIVE_TYPE_8050:
      case DRIVE_TYPE_8250:
        if (drive->type != type) {
            drive->current_half_track = 2 * 18;
            if ((type == DRIVE_TYPE_1001)
                || (type == DRIVE_TYPE_8050)
                || (type == DRIVE_TYPE_8250)) {
                drive->current_half_track = 2 * 38;
            }
        }
        drive->type = type;
        if (drive_true_emulation) {
            drive->enable = 1;
            drive_enable(drive_context[0]);
            /* 1551 drive does not use the IEC bus */
            machine_bus_status_drivetype_set(8, drive_check_bus(type, 0,
                                             IEC_BUS_IEC));
        }
        drive_set_disk_drive_type(type, drive_context[0]);
        driverom_initialize_traps(drive);
        machine_drive_idling_method(0);
        return 0;
      case DRIVE_TYPE_NONE:
        drive->type = type;
        drive_disable(drive_context[0]);
        machine_bus_status_drivetype_set(8, 0);
        return 0;
      default:
        return -1;
    }
}

static int drive1_resources_type(int val, void *param)
{
    unsigned int type, dnr;
    int busses;
    drive_t *drive, *drive0;

    dnr = (unsigned int)param;
    drive = drive_context[dnr]->drive;
    drive0 = drive_context[0]->drive;

    type = (unsigned int)val;
    busses = iec_available_busses();

    /* if bus for drive type is not allowed, set to default value for bus */
    if (!drive_check_bus(type, dnr, busses)) {
        if (busses & IEC_BUS_IEC) {
            type = DRIVE_TYPE_1541;
        } else
        if (busses & IEC_BUS_IEEE) {
            type = DRIVE_TYPE_2031;
        } else
            type = DRIVE_TYPE_NONE;
    }

    if (drive0->enable && drive_check_dual(drive0->type)) {
        /* dual disk drives disable second emulated unit */
        log_warning(drive->log,
                    "Dual disk drive disables second emulated drive");

        type = DRIVE_TYPE_NONE;
    }

    switch (type) {
      case DRIVE_TYPE_1541:
      case DRIVE_TYPE_1541II:
      case DRIVE_TYPE_1551:
      case DRIVE_TYPE_1570:
      case DRIVE_TYPE_1571:
      case DRIVE_TYPE_1571CR:
      case DRIVE_TYPE_1581:
      case DRIVE_TYPE_2031:
      case DRIVE_TYPE_1001:
      case DRIVE_TYPE_2040:
      case DRIVE_TYPE_3040:
      case DRIVE_TYPE_4040:
      case DRIVE_TYPE_8050:
      case DRIVE_TYPE_8250:
        if (drive->type != type) {
            drive->current_half_track = 2 * 18;
            if ((type == DRIVE_TYPE_1001)
                || (type == DRIVE_TYPE_8050)
                || (type == DRIVE_TYPE_8250)) {
                drive->current_half_track = 2 * 38;
            }
        }
        drive->type = type;
        if (drive_true_emulation) {
            drive->enable = 1;
            drive_enable(drive_context[dnr]);
            /* 1551 drive does not use the IEC bus */
            machine_bus_status_drivetype_set(dnr + 8, drive_check_bus(type,
                                             dnr, IEC_BUS_IEC));
        }
        drive_set_disk_drive_type(type, drive_context[dnr]);
        driverom_initialize_traps(drive);
        machine_drive_idling_method(dnr);
        return 0;
      case DRIVE_TYPE_NONE:
        drive->type = type;
        drive_disable(drive_context[dnr]);
        machine_bus_status_drivetype_set(dnr + 8, 0);
        return 0;
      default:
        return -1;
    }
}

static int drive_resources_type(int val, void *param)
{
    switch ((unsigned int)param) {
      case 0:
        return drive0_resources_type(val, param);
      case 1:
      case 2:
      case 3:
        return drive1_resources_type(val, param);
    }

    return -1;
}


static resource_int_t res_drive_type[] = {
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, drive_resources_type, NULL },
    { NULL }
};


int drive_resources_type_init(unsigned int default_type)
{
    unsigned int dnr, type;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;
        if (dnr == 0)
            type = default_type;
        else
            type = DRIVE_TYPE_NONE;

        res_drive_type[0].name = lib_msprintf("Drive%iType", dnr + 8);
        res_drive_type[0].factory_value = (int)type;
        res_drive_type[0].value_ptr = (int *)&(drive->type);
        res_drive_type[0].param = (void *)dnr;

        if (resources_register_int(res_drive_type) < 0)
            return -1;

        lib_free((char *)(res_drive_type[0].name));
    }

    return 0;
}


static const resource_int_t resources_int[] = {
    { "DriveTrueEmulation", 1, RES_EVENT_STRICT, (resource_value_t)1,
      &drive_true_emulation, set_drive_true_emulation, NULL },
    { NULL }
};

static resource_int_t res_drive[] = {
    { NULL, DRIVE_EXTEND_NEVER, RES_EVENT_SAME, NULL,
      NULL, set_drive_extend_image_policy, NULL },
    { NULL }
};

int drive_resources_init(void)
{
    unsigned int dnr;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;

        res_drive[0].name = lib_msprintf("Drive%iExtendImagePolicy", dnr + 8);
        res_drive[0].value_ptr = (int *)&(drive->extend_image_policy);
        res_drive[0].param = (void *)dnr;

        if (resources_register_int(res_drive) < 0)
            return -1;

        lib_free((char *)(res_drive[0].name));
    }

    return machine_drive_resources_init()
        | resources_register_int(resources_int);
}

void drive_resources_shutdown(void)
{
    machine_drive_resources_shutdown();
}

