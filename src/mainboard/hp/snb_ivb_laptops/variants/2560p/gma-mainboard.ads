-- SPDX-License-Identifier: GPL-2.0-or-later

with HW.GFX.GMA;
with HW.GFX.GMA.Display_Probing;

use HW.GFX.GMA;
use HW.GFX.GMA.Display_Probing;

private package GMA.Mainboard is

   ports : constant Port_List :=
     (DP1, -- both on board and dock DP are DP1/HDMI1
      HDMI1,
      Analog,
      LVDS,
      others => Disabled);

end GMA.Mainboard;
