/* SPDX-License-Identifier: GPL-2.0-only */

#include <boardid.h>
#include <ec/acpi/ec.h>
#include <stdint.h>

#include "board_id.h"

/* Get Board ID via EC I/O port write/read */
int get_board_id(void)
{
	int id = -1;

	if (id < 0) {
		uint8_t buffer[2];
		uint8_t index;
		if (send_ec_command(EC_FAB_ID_CMD) == 0) {
			for (index = 0; index < sizeof(buffer); index++)
				buffer[index] = recv_ec_data();
			id = (buffer[0] << 8) | buffer[1];
		}
	}
	return id;
}
