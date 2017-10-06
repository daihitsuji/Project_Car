/*
 * can.c
 *
 *  Created on: 19 sept. 2017
 *      Author: L.Senaneuch
 */

#include <stm32f10x.h>
#include <string.h>


#include "can.h"
#include "gpio.h"
#include "data_interface.h"
#include "system_time.h"

/* Global Variable*/
uint16_t counterCanMes = 0;
uint16_t timeRcv[120];
uint8_t CAN_ENABLE  = DISABLE;

/* Local Variable */
	CAN_InitTypeDef        CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	CanRxMsg RxMessage;
  

/* Local function prototype*/
void CAN_ClockEnable(void);
int can_error_transmit (int id, char *data);


/* Public function */
void CAN_QuickInit(void) {
	
	CAN_ClockEnable();
	GPIO_QuickInit(GPIOB,GPIO_Pin_8,GPIO_Mode_IPU);
	GPIO_QuickInit(GPIOB,GPIO_Pin_9,GPIO_Mode_AF_PP);
	
	GPIO_PinRemapConfig(GPIO_Remap1_CAN1 , ENABLE); // A supprimer 
	
	
	CAN_DeInit(CAN1);

	CAN_StructInit(&CAN_InitStructure);
	CAN_InitStructure.CAN_Prescaler =6; // Prescaler for 600Kbs

	CAN_Init(CAN1,&CAN_InitStructure);

	CAN_InitStructure.CAN_TTCM = DISABLE;
	CAN_InitStructure.CAN_ABOM = DISABLE;
	CAN_InitStructure.CAN_AWUM = DISABLE;
	CAN_InitStructure.CAN_NART = DISABLE;
	CAN_InitStructure.CAN_RFLM = DISABLE;
	CAN_InitStructure.CAN_TXFP = DISABLE;;
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;

	/*CAN Baudrate = 600Kbs*/

	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
	CAN_InitStructure.CAN_BS1 = CAN_BS1_16tq;
	CAN_InitStructure.CAN_BS2 = CAN_BS2_8tq;
	CAN_InitStructure.CAN_Prescaler = 4;
	CAN_Init(CAN1, &CAN_InitStructure);

	CAN_FilterInitStructure.CAN_FilterNumber = 0;

	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);

	/* Transmit */

	CAN_IT_Config();
	CAN_ENABLE = ENABLE;

}

void USB_LP_CAN1_RX0_IRQHandler(void) {
	if(counterCanMes < 120){
		timeRcv[counterCanMes] = time_millis;
		CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
		counterCanMes++;
	}
	else{
		CAN_Send(666,"FIN");
	}
}

/*
 * @brief Send message
 * @param Char array data
 * @param Id to send the data 
 */
int CAN_Send(int id,char *data){
	uint8_t i;
	if(can_error_transmit(id, data) == 0) {
		CanTxMsg TxMessage;
		TxMessage.StdId = id;
		TxMessage.ExtId = 0x01;
		TxMessage.RTR = CAN_RTR_DATA;
		TxMessage.IDE = CAN_ID_STD;
		TxMessage.DLC = 8;
		for(i = 0;i<8;i++)
			TxMessage.Data[i] = data[i];
		
		return CAN_Transmit(CAN1, &TxMessage);
		
	}
	else {
			return ERROR;
	}
}

void CAN_IT_Config(void){
	  NVIC_InitTypeDef  NVIC_InitStructure;
	
		CAN_ITConfig(CAN1, CAN_IT_FMP0 , ENABLE);
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
 
		NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
}

void CAN_ClockEnable(void) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);// A DELETE DANS LE PROJET PRINCIPAL ?
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // A DELETE DANS LE PROJET PRINCIPAL ?
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
}

/*@brief check i	d (11 bits) and data siez (<18 bits) */
int can_error_transmit (int id, char *data) {
		uint8_t length = strlen(data);
	
		if(id < 0 || id > 2048){
				return ERROR_ID;
		}
		if(length>8){
			return ERROR_DLC;
		}
		else {
			return 0; 
		}
}

void CAN_Send_Speed(void)
{
			data_paquet paquet;	
			paquet.floatMessage[0] = pDataITF_STM->wheel_speed_L;
			paquet.floatMessage[1] = pDataITF_STM->wheel_speed_R;
			CAN_Send(ID_SPEED, paquet.stringMessage);
			
}

void CAN_Send_Distance(void)
{
			data_paquet paquet;	
			paquet.floatMessage[0] = pDataITF_STM->travelled_distance_L;
			paquet.floatMessage[1] = pDataITF_STM->travelled_distance_R;
			CAN_Send(ID_DISTANCE, paquet.stringMessage);
}

void CAN_Send_Sensors(void)
{
	// Todo : Change array index by US contant in us_sensor.h
			data_paquet paquet;			
			paquet.byteMessage[0] = pDataITF_STM->ultrasonic_sensors[0];
			paquet.byteMessage[1] = pDataITF_STM->ultrasonic_sensors[1];
			paquet.byteMessage[2] = pDataITF_STM->ultrasonic_sensors[2];
			paquet.byteMessage[3] = pDataITF_STM->ultrasonic_sensors[3];
			paquet.byteMessage[4] = pDataITF_STM->ultrasonic_sensors[4];
			paquet.byteMessage[5] = pDataITF_STM->ultrasonic_sensors[5];
			paquet.byteMessage[6] = pDataITF_STM->battery_level;
			paquet.byteMessage[7] = pDataITF_STM->steering_stop_sensor_L;
			CAN_Send(ID_SENSORS, paquet.stringMessage);
}

void CAN_Send_Wheel_Position(void){
			data_paquet paquet;
			paquet.byteMessage[0] = pDataITF_STM->wheel_SENSOR_L;
			paquet.byteMessage[0] = pDataITF_STM->wheel_SENSOR_R;
			CAN_Send(ID_POSITION, paquet.stringMessage);
}

void CAN_Send_Current(void){
			data_paquet paquet;
			paquet.byteMessage[0] = pDataITF_STM->motor_current_L;
			paquet.byteMessage[0] = pDataITF_STM->motor_current_R;
			CAN_Send(ID_MOTOR_CURRENT, paquet.stringMessage);
}

void CAN_Rx_Callback(uint8_t size, int id, char * data) {
		data_paquet paquet;
		uint8_t i;
		for(i = 0; i < size ; i++) {
				paquet.byteMessage[i] = data[i];
		}
		if(id == ID_MOTOR_PROP) {
				pDataITF_PI->motor_prop = paquet.intMessage[0];
		}
		else if (id == ID_MOTOR_DIR) {
			pDataITF_PI->motor_dir = paquet.intMessage[0];
			
		}
		else if(id == ID_MOTOR_ENABLE){
				pDataITF_PI->enable_motors_control = paquet.byteMessage[0];
		}
}
