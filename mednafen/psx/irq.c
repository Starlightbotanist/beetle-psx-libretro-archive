/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdint.h>

#include "../mednafen-types.h"
#include "../state.h"
#include "../state_helpers.h"

#include "irq.h"
#include "../../osd_message.h"
#include "cpu.h"

static uint16_t Asserted;
static uint16_t Mask;
static uint16_t Status;
static bool irq_diag_armed;
static unsigned irq_diag_events;

extern retro_log_printf_t log_cb;

void IRQ_DiagArm(void)
{
   irq_diag_armed = true;
   irq_diag_events = 0;
   log_cb(RETRO_LOG_INFO, "[IRQDIAG] armed\n");
}

static void IRQ_DiagLog(const char *event, int which, bool level,
      uint32_t address, uint32_t value)
{
   if(!irq_diag_armed || irq_diag_events >= 2048)
      return;

   log_cb(RETRO_LOG_INFO,
         "[IRQDIAG] seq=%u event=%s which=%d level=%d addr=%08x "
         "value=%08x asserted=%04x status=%04x mask=%04x pending=%04x\n",
         irq_diag_events++, event, which, level, address, value,
         Asserted, Status, Mask, Status & Mask);
}

void IRQ_Power(void)
{
   Asserted = 0;
   Status = 0;
   Mask = 0;

   CPU_AssertIRQ(0, (bool)(Status & Mask));
}

int IRQ_StateAction(void *data, int load, int data_only)
{
   static SFORMAT StateRegs[] =
   {
      SFVARN(Asserted, "Asserted"),
      SFVARN(Mask, "Mask"),
      SFVARN(Status, "Status"),
      SFEND
   };
   int ret = MDFNSS_StateAction(data, load, data_only, StateRegs, "IRQ");

   if(load)
   {
      CPU_AssertIRQ(0, (bool)(Status & Mask));
   }

   return(ret);
}

void IRQ_Assert(int which, bool status)
{
   uint32_t old_Asserted = Asserted;

   Asserted &= ~(1 << which);

   if(status)
   {
      Asserted |= 1 << which;
      Status   |= (old_Asserted ^ Asserted) & Asserted;
   }

   if(old_Asserted != Asserted)
      IRQ_DiagLog("assert", which, status, 0, 0);

   CPU_AssertIRQ(0, (bool)(Status & Mask));
}


void IRQ_Write(uint32_t A, uint32_t V)
{
   /* FIXME if we ever have "accurate" bus emulation */
   V <<= (A & 3) * 8;

   if(A & 4)
      Mask = V;
   else
      Status &= V;

   IRQ_DiagLog(A & 4 ? "mask_write" : "status_write", -1, false, A, V);

   CPU_AssertIRQ(0, (bool)(Status & Mask));
}


uint32_t IRQ_Read(uint32_t A)
{
   uint32_t ret = Status;

   if(A & 4)
      ret = Mask;

   /* FIXME: Might want to move this out to psx.cpp eventually. */
   ret |= 0x1F800000;
   ret >>= (A & 3) * 8;

   return(ret);
}

void IRQ_DiagState(uint16_t *asserted, uint16_t *status, uint16_t *mask)
{
   *asserted = Asserted;
   *status = Status;
   *mask = Mask;
}
