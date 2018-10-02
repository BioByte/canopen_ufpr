/*
This file is part of CanFestival, a library implementing CanOpen Stack.

Copyright (C): Edouard TISSERANT and Francis DUPIN
AVR Port: Andreas GLAUSER and Peter CHRISTEN

See COPYING file for copyrights details.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//#define DEBUG_WAR_CONSOLE_ON
//#define DEBUG_ERR_CONSOLE_ON

#include "due_can.h"
#include "canfestival.h"

volatile unsigned char msg_received = 0;

unsigned char canInit(unsigned int bitrate) /*É PRA ESTAR FEITO*/
/******************************************************************************
Initialize the hardware to receive CAN messages and start the timer for the
CANopen stack.
INPUT	bitrate		bitrate in kilobit
OUTPUT	1 if successful	
******************************************************************************/
{ 
		//setup port0 hardware
		if (CAN.init(bitrate))
		{
			// Disable all CAN0 & CAN1 interrupts
			CAN.disable_interrupt(CAN_DISABLE_ALL_INTERRUPT_MASK);
			//DEBUG THIS TO FIGURE OUT PROPER IRQ
			NVIC_EnableIRQ(CAN0_IRQn);

			//reset mailboxes
			CAN.reset_all_mailbox();

            //setup the masks for the recieve mailbox
            CAN.mailbox_set_accept_mask(0, MAM_mask, MID_mask > 0x7FF ? true : false);
            CAN.mailbox_set_id         (0, MID_mask, MID_mask > 0x7FF ? true : false);


			//setup the receive mailbox for CAN 0 to mailbox 0
			CAN.mailbox_set_mode(0, CAN_MB_RX_MODE); 

			//setup the transmit mailbox for CAN 0 to mailbox 1
			CAN.mailbox_set_mode(1, CAN_MB_TX_MODE);
			CAN.mailbox_set_priority(1, 15);
			CAN.mailbox_set_datalen(1, 8);



			//enable RX interrupt for mailbox0
			CAN.enable_interrupt(CAN_IER_MB0);

      return 1;
		}
    else
      return 0;
}

unsigned char canSend(CAN_PORT notused, Message *m) /*É PRA ESTAR FEITO*/
/******************************************************************************
The driver send a CAN message passed from the CANopen stack
INPUT	CAN_PORT is not used (only 1 avaiable)
	Message *m pointer to message to send
OUTPUT	1 if  hardware -> CAN frame
******************************************************************************/
{
	for (int i = 0; i < 8; i++) {
		if (((m_pCan->CAN_MB[i].CAN_MMR >> 24) & 7) == CAN_MB_TX_MODE)
		{//is this mailbox set up as a TX box?
			if (m_pCan->CAN_MB[i].CAN_MSR & CAN_MSR_MRDY) 
			{//is it also available (not sending anything?)
				mailbox_set_id(i, m->cob_id, 0);
				mailbox_set_datalen(i, m->len);
				//mailbox_set_priority(i, txFrame.priority); NAO TEM NA STRUCT DO FESTIVINO
				for (uint8_t cnt = 0; cnt < 8; cnt++)
				{    
					mailbox_set_databyte(i, cnt, m->data[cnt]);
				}       
				enable_interrupt(0x01u << i); //enable the TX interrupt for this box
				global_send_transfer_cmd((0x1u << i));
				return 1; //we've sent it. mission accomplished.
			}
		}
    }
	
    //if execution got to this point then no free mailbox was found above
    //so, queue the frame if possible. But, don't increment the 
	//tail if it would smash into the head and kill the queue.
	uint8_t temp;
	temp = (tx_buffer_tail + 1) % SIZE_TX_BUFFER;
	if (temp == tx_buffer_head) return 0;
    tx_frame_buff[tx_buffer_tail].id = m->cob_id;
   // tx_frame_buff[tx_buffer_tail].extended = txFrame.extended;  NAO TEM EXTENDED NA STRUCT DO FESTIVINO CAN;H
    tx_frame_buff[tx_buffer_tail].length = m->len;
    tx_frame_buff[tx_buffer_tail].data.value = m->data;
    tx_buffer_tail = temp;
	return 1;
}

unsigned char canReceive(Message *m)  /*É PRA ESTAR FEITO*/
/******************************************************************************
The driver pass a received CAN message to the stack
INPUT	Message *m pointer to received CAN message
OUTPUT	1 if a message received
******************************************************************************/
{
 if (rx_buffer_head == rx_buffer_tail) return 0;
	m->cob_id = rx_frame_buff[rx_buffer_tail].id;
	//buffer.extended = rx_frame_buff[rx_buffer_tail].extended;
	m->len = rx_frame_buff[rx_buffer_tail].length;
	m->data = rx_frame_buff[rx_buffer_tail].data.value;
	rx_buffer_tail = (rx_buffer_tail + 1) % SIZE_RX_BUFFER;
return 1;
}






/*
unsigned char canChangeBaudRate_driver( CAN_HANDLE fd, char* baud)
{

	return 0;
}

#ifdef  __IAR_SYSTEMS_ICC__
#pragma type_attribute = __interrupt
#pragma vector=CANIT_vect
void CANIT_interrupt(void)
#else	// GCC
ISR(CANIT_vect)
#endif	// GCC
*****************************************************************************
CAN Interrupt
*****************************************************************************
{
  unsigned char saved_page = CANPAGE;
  unsigned char i;

  if (CANGIT & (1 << CANIT))	// is a messagebox interrupt
  {
    if ((CANSIT1 & TX_INT_MSK) == 0)	// is a Rx interrupt
    {
      for (i = 0; (i < NB_RX_MOB) && (CANGIT & (1 << CANIT)); i++)	// Search the first MOb received
      {
        Can_set_mob(i);			// Change to MOb
        if (CANSTMOB & MOB_RX_COMPLETED)	// receive ok
        {
          Can_clear_status_mob();	// Clear status register
	  Can_mob_abort();		// disable the MOb = received
	  msg_received++;
        }
        else if (CANSTMOB & ~MOB_RX_COMPLETED)	// error
        {
          Can_clear_status_mob();	// Clear status register
	  Can_config_rx_buffer();	// reconfigure as receive buffer
        }
      }
    }
    else				// is a Tx interrupt	 
    {
      for (i = NB_RX_MOB; i < NB_MOB; i++)	// Search the first MOb transmitted
      {
        Can_set_mob(i);			// change to MOb
        if (CANSTMOB)			// transmission ok or error
        {
          Can_clear_status_mob();	// clear status register
	  CANCDMOB = 0;			// disable the MOb
	  break;
        }
      }
    }
  }

  CANPAGE = saved_page;

  // Bus Off Interrupt Flag
  if (CANGIT & (1 << BOFFIT))    // Finaly clear the interrupt status register
  {
    CANGIT |= (1 << BOFFIT);                    // Clear the interrupt flag
  }
  else
    CANGIT |= (1 << BXOK) | (1 << SERG) | (1 << CERG) | (1 << FERG) | (1 << AERG);// Finaly clear other interrupts
}

#ifdef  __IAR_SYSTEMS_ICC__
#pragma type_attribute = __interrupt
#pragma vector=OVRIT_vect
void OVRIT_interrupt(void)
#else	// GCC
ISR(OVRIT_vect)
#endif	// GCC
*****************************************************************************
CAN Timer Interrupt
*****************************************************************************
{
  CANGIT |= (1 << OVRTIM);
}
*/
