
#include "mbed.h"
#include "ESP8266.h"               // Include header file from Author: Antonio Quevedo
#include "math.h"
#include "DHT.h"
#include "GT511C3.hpp"
#include <string>

#define APIKEY JJAOBK32WOINKT00    //Put "Write key" of your channel in thingspeak.com 
#define IP "184.106.153.149"       // IP Address of "api.thingspeak.com\"
#define WIFI_SSID "Redmi"
#define WIFI_PASS "akash12345"
#define FPS_ENROLL_PASS "Rags\n"

Serial FRDM_UART_Debug(USBTX,USBRX);

ESP8266 ESP_8266_UART(PTC15, PTC14, 115200); // UART for ESP8266 Wifi module
// Options are TX-RX - PTB11 - PTB10 , PTC17 - PTC16 , PTC15 - PTC14

SPI SPI_Bus(PTD2,PTD3,PTD1); // (MOSI MISO CLK)setup SPI interface
DigitalOut SPI_CS_AMM(PTA2);
DigitalOut SPI_CS_VOLT(PTB9);

I2C I2C_Bus(PTE25,PTE24);

AnalogIn AN_Thermo(PTB2); // Thermocouple Analog Input

DigitalIn DG_Motion(PTC2); // Motion module Digital Input

DHT DHT_Temp_Hum(PTC4,DHT22); //DHT Sensor

GT511C3 FPS(PTC17, PTC16);

const int Light_I2C_Addr = 0x88;

char ESP_8266_CMD_Send[255],ESP_8266_CMD_Recv[1000];           //ESP_8266_CMD_Send = string used to send command to ESP8266 &  ESP_8266_CMD_Recv = string used to receive response from ESP8266

float Amm_Out = 0;
float Volt_Out = 0;
float Light_Out = 0;
float Thermo_Out = 0;
float Temp_Out = 0;
float Hum_Out = 0;
int Motion_Out = 0;
int Finger_Out = 0;
float Pres_Out = 0;
bool FPS_Auth = false;


void ESP_8266_Init(void); // Function used to initialize ESP8266 wifi module 
void ESP_8266_TX_Data(void);       // Function used to connect with thingspeak.com and update channel using ESP8266 wifi module 
void FPS_Func(void);
int FPS_Wait_Time(int press,bool Det_Mot, unsigned long *ms_time);

int main() 
{
    int SPI_High_byte = 0;
    int SPI_Low_byte = 0; 
    float Temp_f_1 = 0.00;
    float Temp_f_2 = 0.00;
    int Temp_i_1 = 0;
    int Temp_i_2 = 0;
    int Temp_i_3 = 0;
    int Temp_i_4 = 0;
    char I2C_Cmd[3];
    int loop_count = 0;
    int FPS_Ret = 0;
    int FPS_Attempt = 1;
    int FPS_Enroll_ID = 0;
    unsigned long FPS_Param = 0;
    unsigned short FPS_Resp = 0;
    char FPS_Enroll_Pass[20];
    unsigned long FPS_Delay = 0;
    
    FRDM_UART_Debug.baud(115200);           // Baud rate used for communicating with Tera-term on PC
    
    ESP_8266_Init();
    
    SPI_Bus.format(8,0);
    SPI_Bus.frequency(1000000);
    
    I2C_Bus.frequency(100000); // set required i2c frequency
    
    FPS_Ret = FPS.Open();
    if(FPS_Ret == -1)
    {
        FRDM_UART_Debug.printf("FPS NACK Open\r\n");  
    }
    else
    {
        FRDM_UART_Debug.printf("FPS Init\r\n");  
        FRDM_UART_Debug.printf("FPS F/W = %d , ISO_Size = %d , Serial Num = %s\r\n",FPS.FirmwareVersion,FPS.IsoAreaMaxSize,FPS.DeviceSerialNumber);  
    }      
    
    FPS_Ret = FPS.CmosLed(1);
    if(FPS_Ret == -1)
    {
        FRDM_UART_Debug.printf("FPS NACK LED Set\r\n");  
    }

    wait(1);
    
    if(DG_Motion == 1)
    {
        FRDM_UART_Debug.printf("Motion detected, press Finger within 5 seconds to Start\r\n");  
        
        FPS_Delay = 5000;
        FPS_Ret = FPS_Wait_Time(1,false,&FPS_Delay);
        if(FPS_Ret == 1)
        {
            FPS_Func();
        }
    }
    FRDM_UART_Debug.printf("Start sampling data\r\n");  // Starting point
    
    while (1) 
    {
        Amm_Out = 0;
        Volt_Out = 0;
        Light_Out = 0;
        Thermo_Out = 0;
        Temp_Out = 0;
        Hum_Out = 0;
        Motion_Out = 0;
        
        // Copy Motion values
        
        Motion_Out = DG_Motion;
        
        // Ammeter
        
        SPI_High_byte = 0;
        SPI_Low_byte = 0; 
        Temp_f_1 = 0.00;
        Temp_f_2 = 0.00;
        Temp_i_1 = 0;
        Temp_i_2 = 0;
        Temp_i_3 = 0;
        I2C_Cmd[0] = 0;
        I2C_Cmd[1] = 0;
        I2C_Cmd[2] = 0;
        loop_count = 0;
        
        SPI_CS_AMM = 0;
        
        SPI_High_byte = SPI_Bus.write(0);
        SPI_Low_byte = SPI_Bus.write(0);
        
        SPI_CS_AMM = 1;
        
        Temp_f_1 = (( SPI_High_byte & 0x1F ) << 7 ) | (( SPI_Low_byte >> 1 ));
            
        Temp_f_2= (float)(( Temp_f_1 * 1.00 ) / 4096.00 ); // Converting to volts
        
        Amm_Out = (float)(( Temp_f_2 - 0.50 ) * 1000.00);
        
        if(FPS_Auth)
            FRDM_UART_Debug.printf("Current value = %f mA\r\n", Amm_Out);
        wait_ms(100);
        
        // Voltmeter
        
        SPI_High_byte = 0;
        SPI_Low_byte = 0; 
        Temp_f_1 = 0.00;
        Temp_f_2 = 0.00;
        Temp_i_1 = 0;
        Temp_i_2 = 0;
        Temp_i_3 = 0;
        I2C_Cmd[0] = 0;
        I2C_Cmd[1] = 0;
        I2C_Cmd[2] = 0;
        loop_count = 0;  
        
        SPI_CS_VOLT = 0;

        SPI_High_byte = SPI_Bus.write(0);
        SPI_Low_byte = SPI_Bus.write(0);
        
        SPI_CS_VOLT = 1;
        
        Temp_f_1 = ((SPI_High_byte & 0x1f) << 7) | ((SPI_Low_byte >> 1));
        
        Temp_f_2 = (float)((Temp_f_1 * 33) / 4096); // show value in volts.
        
        Volt_Out = (float)(Temp_f_2 - 16.5);
        
        if(FPS_Auth)
            FRDM_UART_Debug.printf("Voltage value = %f V\r\n", Volt_Out);
        wait_ms(100);
        
         //ambient light 
         
        SPI_High_byte = 0;
        SPI_Low_byte = 0; 
        Temp_f_1 = 0.00;
        Temp_f_2 = 0.00;
        Temp_i_1 = 0;
        Temp_i_2 = 0;
        Temp_i_3 = 0;
        I2C_Cmd[0] = 0;
        I2C_Cmd[1] = 0;
        I2C_Cmd[2] = 0;
        loop_count = 0;
        
        I2C_Cmd[0] = 0x01;   //configuration register
        I2C_Cmd[1]= 0xCC;    //configuration data
        I2C_Cmd[2]= 0x01;   //configuration data
        
        I2C_Bus.write(Light_I2C_Addr, I2C_Cmd, 3);
        
        I2C_Cmd[0] = 0x00; // data register
        
        I2C_Bus.write(Light_I2C_Addr, I2C_Cmd, 1);
        
        wait_ms(100);
        
        I2C_Bus.read(Light_I2C_Addr, I2C_Cmd, 2);
     
        Temp_i_1= I2C_Cmd[0]>>4;
        Temp_i_2= (I2C_Cmd[0]-(Temp_i_1<<4))*256+I2C_Cmd[1];

        for(loop_count = 0,Temp_i_3 = 1 ; loop_count < Temp_i_1 ; Temp_i_3 = Temp_i_3 * 2,loop_count++);
        
        Light_Out= (Temp_i_2 * Temp_i_3) / 100;
        
        if(FPS_Auth)
            FRDM_UART_Debug.printf("Lux = %.2f\n\r", Light_Out);  
        wait_ms(100); 
        
        // Thermocouple
        
        SPI_High_byte = 0;
        SPI_Low_byte = 0; 
        Temp_f_1 = 0.00;
        Temp_f_2 = 0.00;
        Temp_i_1 = 0;
        Temp_i_2 = 0;
        Temp_i_3 = 0;
        I2C_Cmd[0] = 0;
        I2C_Cmd[1] = 0;
        I2C_Cmd[2] = 0;
        loop_count = 0;
        
        Temp_f_1 = AN_Thermo.read_u16();
        
        Temp_f_1 = (( Temp_f_1 / 65536 ) * 330);
        
        Thermo_Out = Temp_f_1;
        
        if(FPS_Auth)
            FRDM_UART_Debug.printf("Thermocouple volt diff = %.2f C\r\n",Thermo_Out);
        wait_ms(100); 
        
        // Temp and Humidity
        
        SPI_High_byte = 0;
        SPI_Low_byte = 0; 
        Temp_f_1 = 0.00;
        Temp_f_2 = 0.00;
        Temp_i_1 = 0;
        Temp_i_2 = 0;
        Temp_i_3 = 0;
        I2C_Cmd[0] = 0;
        I2C_Cmd[1] = 0;
        I2C_Cmd[2] = 0;
        loop_count = 0;
        
        Temp_i_1 = DHT_Temp_Hum.readData();
        if (Temp_i_1 == 0)  // Read success
        {
            //wait_ms(1000);
            Temp_f_1 = DHT_Temp_Hum.ReadTemperature(FARENHEIT);
            Temp_f_2 = DHT_Temp_Hum.ReadHumidity();
        }
        else // Read failure
        {
            Temp_f_1 = 0;
            Temp_f_2 = 0;
        }
        
        Temp_Out = Temp_f_1;
        Hum_Out = Temp_f_2;
        
        if(FPS_Auth)
            FRDM_UART_Debug.printf("Temperature = %4.2f F ,  Humidity = %4.2f \r\n",Temp_Out,Hum_Out);
        wait_ms(100); 
        
        //if(FPS_Auth)
        FRDM_UART_Debug.printf("Sending Data to Server\r\n");
        ESP_8266_TX_Data(); 
        
        FPS_Delay = 15000;
        
FPS_Check_Again:        
        FPS_Ret = FPS_Wait_Time(1,true,&FPS_Delay);
        if(FPS_Ret == 1)
        {
            FRDM_UART_Debug.printf("Motion detected, press Finger within %d seconds to Authenticate and display data\r\n",FPS_Delay/1000);  
            FPS_Ret = FPS_Wait_Time(1,false,&FPS_Delay);
            if(FPS_Ret == 1)
            {
                FPS_Func();
                goto FPS_Check_Again;
            }
            else if(FPS_Delay > 99)
            {
                FRDM_UART_Debug.printf("Remaining delay is %d\r\n",FPS_Delay);
                goto FPS_Check_Again;
            }
        }
        else if(FPS_Delay > 99)
        {
            FRDM_UART_Debug.printf("Remaining delay is %d\r\n",FPS_Delay);
            goto FPS_Check_Again;
        }

        //wait(15);
        
    }
}

int FPS_Wait_Time(int press,bool Det_Mot, unsigned long *ms_time)
{
    for(;*ms_time>99;)
    {
        if((FPS.IsPress() == press) || ((Det_Mot == true) && (DG_Motion == 1)))
            return 1;
        else
        {
            *ms_time-=100;
            wait_ms(100);
        }
    }
    return 0;
    /*
    for(;(FPS.IsPress() != press);wait_ms(100),*ms_time-=100);
    if(*ms_time >= 99)
        return 1;
    else
        return 0;
    */
}

void FPS_Func(void)
{
    int Temp_i_1 = 0;
    int Temp_i_2 = 0;
    int Temp_i_3 = 0;
    int Temp_i_4 = 0;
    int loop_count = 0;
    int FPS_Ret = 0;
    int FPS_Attempt = 1;
    int FPS_Enroll_ID = 0;
    unsigned long FPS_Param = 0;
    unsigned short FPS_Resp = 0;
    unsigned long FPS_Delay = 0;
    char FPS_Enroll_Pass[20];
    
    FRDM_UART_Debug.printf("FPS Select Option:\r\n1. Verify ID \r\n2. Enroll ID\r\n3. Delete ID \r\n4. Quit \r\n"); 
    FRDM_UART_Debug.scanf("%d",&Temp_i_4);
    
    switch(Temp_i_4)
    {
        case 1:
        
            FRDM_UART_Debug.printf("FPS VERIFICATION\r\n"); 
            FRDM_UART_Debug.printf("FPS Press finger to start\r\n"); 
            
            FPS_Delay = 10000;
            FPS_Wait_Time(1,false,&FPS_Delay);
            
            //Verify 
            FPS_Ret = FPS.Capture(1);
            if(FPS_Ret == -1)
            {
                FRDM_UART_Debug.printf("FPS Verification failed, place finger properly\r\n");  
                FPS_Auth = false;
                goto loop_end;
            }
            
            FRDM_UART_Debug.printf("FPS Captured\r\n");  
            
            FPS_Ret = FPS.Identify();
            if(FPS_Ret != -1)
            {
                FRDM_UART_Debug.printf("FPS Authentication PASSED with ID = %d \r\n",FPS_Ret); 
                FPS_Auth = true;
                goto loop_end;
            }
            else
            {
                FRDM_UART_Debug.printf("FPS Authentication FAILED \r\n"); 
                FPS_Auth = false;
                goto loop_end;    
            }
        break;
        
        case 2:
            FRDM_UART_Debug.printf("FPS ENROLL\r\n"); 

            for(FPS_Attempt = 3;FPS_Attempt >= 1;FPS_Attempt --)
            {
                FRDM_UART_Debug.printf("FPS Enter Enroll passoword\r\n"); 
                FRDM_UART_Debug.scanf("%s",&FPS_Enroll_Pass[0]); 
                
                if(FPS_Enroll_Pass == FPS_ENROLL_PASS)
                {
                   FRDM_UART_Debug.printf("FPS Wrong Enroll passoword %d attempts left\r\n",FPS_Attempt);     
                }
                else
                    break;
            }

            if(FPS_Attempt < 1)
            {
                FRDM_UART_Debug.printf("FPS Enroll Password authentication failed \r\n"); 
                goto loop_end;
            }

            FRDM_UART_Debug.printf("FPS Enroll passoword Authenticated\r\n");
            
            for(FPS_Enroll_ID = 0; FPS_Enroll_ID <20 ;FPS_Enroll_ID++)
            {
                FPS_Ret = FPS.CheckEnrolled(FPS_Enroll_ID);
                if(FPS_Ret == 0)      
                    continue;
                else
                    break;
            }

            FRDM_UART_Debug.printf("FPS Enroll ID %d\r\n",FPS_Enroll_ID);

            if(FPS_Enroll_ID < 20)
            {
                FRDM_UART_Debug.printf("FPS Starting enrolling, place finger on Sensor when LED glows\r\n");
                
                FPS_Ret = FPS.CmosLed(0);
                if(FPS_Ret == -1)
                {
                    FRDM_UART_Debug.printf("FPS Enroll Failed try again\r\n"); 
                    goto loop_end;
                }
                    
                FPS_Param = FPS_Enroll_ID;
                
                FPS_Ret = FPS.SendRecv(FPS.CMD_EnrollStart,&FPS_Param,&FPS_Resp);
                if((FPS_Ret != 0) || (FPS_Resp != FPS.CMD_Ack))
                {
                    FRDM_UART_Debug.printf("FPS Enroll Failed try again\r\n"); 
                    goto loop_end;
                }
                
                Temp_i_1 = 1;
                
                for(Temp_i_2 = 1;Temp_i_2 < 10 ; Temp_i_2++)
                {
                    for(;Temp_i_1 <= 3 ; Temp_i_1++,Temp_i_2 = 1)
                    {
                        FPS_Delay = 10000;
                        FPS_Wait_Time(0,false,&FPS_Delay);
                        
                        FPS_Ret = FPS.CmosLed(1);
                        if(FPS_Ret == -1)
                        {
                            FRDM_UART_Debug.printf("FPS Enroll Failed trying again\r\n"); 
                            continue;
                        }
                        
                        for(Temp_i_3 = 1;Temp_i_3 <= 10;Temp_i_3++)
                        {
                        
                            FRDM_UART_Debug.printf("FPS Place finger on Sensor NOW %d\r\n",Temp_i_1);
                            FPS_Delay = 10000;
                            FPS_Wait_Time(1,false,&FPS_Delay);
                            
                            if(FPS.Capture(1) == 0)
                                break;
                                
                            wait_ms(500);
                        }
                        
                        if(Temp_i_2 > 10)
                        {
                            FRDM_UART_Debug.printf("FPS Enroll Failed trying again\r\n"); 
                            continue;
                        }   

                        FPS_Ret = FPS.Enroll_N(Temp_i_1);
                        if(FPS_Ret != 0)
                        {
                            FRDM_UART_Debug.printf("FPS Enroll Failed trying again\r\n"); 
                            continue;
                        }
                            
                        FPS_Ret = FPS.CmosLed(0);
                        if(FPS_Ret == -1)
                        {
                            FRDM_UART_Debug.printf("FPS Enroll Failed trying again\r\n"); 
                            continue;
                        }
                        
                        FRDM_UART_Debug.printf("FPS REMOVE finger on Sensor NOW\r\n");      
                    }
                }
                
                FPS_Ret = FPS.CheckEnrolled(FPS_Enroll_ID);
                if(FPS_Ret == 0)     
                {
                    FRDM_UART_Debug.printf("FPS Enroll PASSED\r\n");       
                    goto loop_end;
                }
                else
                {
                    FRDM_UART_Debug.printf("FPS Enroll FAILED\r\n");       
                    goto loop_end;                       
                }
            }
            else
            {
                FRDM_UART_Debug.printf("FPS All ID's are full, swithcing to ID delete mode\r\n");
                goto delete_fps_id;
            }
        break;
        
        case 3:
            // Delete ID 
delete_fps_id:                  
            FRDM_UART_Debug.printf("FPS DELETE ID, enter option select\r\n1. Delete specific ID \r\n2. Delete All \r\n3. Quit\r\n");
            FRDM_UART_Debug.scanf("%d",&Temp_i_1); 
            
            switch (Temp_i_1)
            {
                case 1:
                case 2:
                        for(FPS_Attempt = 3;FPS_Attempt >= 1;FPS_Attempt --)
                        {
                            FRDM_UART_Debug.printf("FPS Enter Enroll passoword\r\n"); 
                            FRDM_UART_Debug.scanf("%s",&FPS_Enroll_Pass[0]); 
                            
                            if(FPS_Enroll_Pass == FPS_ENROLL_PASS)
                            {
                               FRDM_UART_Debug.printf("FPS Wrong Enroll passoword %d attempts left\r\n",FPS_Attempt);     
                            }
                            else
                                break;
                        }
                        
                        FRDM_UART_Debug.printf("FPS Password Authenticated\r\n");
                        
                        if(Temp_i_1 == 1)
                        {
                            FRDM_UART_Debug.printf("FPS Enter ID to delete\r\n");
                            FRDM_UART_Debug.scanf("%d",&Temp_i_2);
                            
                            
                            FPS_Ret = FPS.DeleteID(Temp_i_2);
                            if(FPS_Ret != 0)
                            {
                                FRDM_UART_Debug.printf("FPS Unable to delete ID %d\r\n",Temp_i_2); 
                                break;
                            }
                            else
                                FRDM_UART_Debug.printf("FPS ID %d has been deleted\r\n",Temp_i_2); 
                        }
                        else if(Temp_i_1 == 2)
                        {
                            for(FPS_Enroll_ID = 0; FPS_Enroll_ID <20 ;FPS_Enroll_ID++)
                            {
                                FPS_Ret = FPS.DeleteID(FPS_Enroll_ID);
                            }
                            
                            for(FPS_Enroll_ID = 0; FPS_Enroll_ID <20 ;FPS_Enroll_ID++)
                            {
                                FPS_Ret = FPS.CheckEnrolled(FPS_Enroll_ID);
                                if(FPS_Ret != 0)      
                                    continue;
                                else
                                    break;
                            }
                
                            if(FPS_Enroll_ID == 20)
                            {
                                FRDM_UART_Debug.printf("FPS All ID's deleted\r\n");
                            }
                            else
                            {
                                FRDM_UART_Debug.printf("FPS Delete ALL failed at ID %d\r\n",FPS_Enroll_ID); 
                            }
                        }
                            
                    break;
                case 3:
                default:
                    goto loop_end;
                    break;      
            } 
                
        break;
        
        case 4:
        default:
                goto loop_end;
    }   
loop_end:
    FRDM_UART_Debug.printf("FPS Loop End reached\r\n");    
    
    FPS_Ret = FPS.CmosLed(1);
    if(FPS_Ret == -1)
    {
        FRDM_UART_Debug.printf("FPS LED ON Failed Loop End\r\n"); 
        //continue;
    }            
 
}
    


void ESP_8266_Init(void)
{    
    FRDM_UART_Debug.printf("Initializing and Reset ESP\r\n"); 
      
    ESP_8266_UART.Reset();                   //RESET ESP
    ESP_8266_UART.RcvReply(ESP_8266_CMD_Recv, 400);        //receive a response from ESP
    //if(FPS_Auth)
        //FRDM_UART_Debug.printf(ESP_8266_CMD_Recv);          //Print the response onscreen 
    wait(2);
    
    strcpy(ESP_8266_CMD_Send,"AT");
    ESP_8266_UART.SendCMD(ESP_8266_CMD_Send);
    if(FPS_Auth)
        FRDM_UART_Debug.printf(ESP_8266_CMD_Send);
    //wait(2);
    ESP_8266_UART.RcvReply(ESP_8266_CMD_Recv, 400);       
    if(FPS_Auth)
        FRDM_UART_Debug.printf(ESP_8266_CMD_Recv);      
    wait(0.1);
    
    strcpy(ESP_8266_CMD_Send,"AT+CWMODE=1");
    ESP_8266_UART.SendCMD(ESP_8266_CMD_Send);
    if(FPS_Auth)
        FRDM_UART_Debug.printf(ESP_8266_CMD_Send);
    wait(2);
    
    
    if(!strcmp(ESP_8266_CMD_Recv,"WIFI CONNECTED"))
    {
        strcpy(ESP_8266_CMD_Send,"AT+CWJAP=\"");
        strcat(ESP_8266_CMD_Send,WIFI_SSID);
        strcat(ESP_8266_CMD_Send,"\",\"");
        strcat(ESP_8266_CMD_Send,WIFI_PASS);
        strcat(ESP_8266_CMD_Send,"\"");
    
        ESP_8266_UART.SendCMD(ESP_8266_CMD_Send);
        //if(FPS_Auth)
            FRDM_UART_Debug.printf(ESP_8266_CMD_Send);
        wait(5);
        ESP_8266_UART.RcvReply(ESP_8266_CMD_Recv, 400);       
        //if(FPS_Auth)
            FRDM_UART_Debug.printf(ESP_8266_CMD_Recv); 
    }
    else 
       FRDM_UART_Debug.printf("Wifi was preconfigured\r\n"); 
        
    strcpy(ESP_8266_CMD_Send,"AT+CIPMUX=0");
    ESP_8266_UART.SendCMD(ESP_8266_CMD_Send);
    //if(FPS_Auth)
        FRDM_UART_Debug.printf(ESP_8266_CMD_Send);
    //wait(2);
    ESP_8266_UART.RcvReply(ESP_8266_CMD_Recv, 400);       
    //if(FPS_Auth)
        FRDM_UART_Debug.printf(ESP_8266_CMD_Recv); 

}

void ESP_8266_TX_Data(void)
{
   
    //ESP updates the Status of Thingspeak channel//
    
    strcpy(ESP_8266_CMD_Send,"AT+CIPSTART=");
    strcat(ESP_8266_CMD_Send,"\"TCP\",\"");
    strcat(ESP_8266_CMD_Send,IP);
    strcat(ESP_8266_CMD_Send,"\",80");
    
    ESP_8266_UART.SendCMD(ESP_8266_CMD_Send); 
    if(FPS_Auth)
        FRDM_UART_Debug.printf("S\r\n%s",ESP_8266_CMD_Send);
    //wait(2);                                                    
    ESP_8266_UART.RcvReply(ESP_8266_CMD_Recv, 1000);
    if(FPS_Auth)
        FRDM_UART_Debug.printf("R\r\n%s",ESP_8266_CMD_Recv);
    wait(1);
    
    /*
        float Amm_Out = 0;
        float Volt_Out = 0;
        float Light_Out = 0;
        float Thermo_Out = 0;
        float Temp_Out = 0;
        float Hum_Out = 0;
        int Motion_Out = 0;
    */
    sprintf(ESP_8266_CMD_Send,"GET https://api.thingspeak.com/update?key=JJAOBK32WOINKT00&field1=%d&field2=%d&field3=%f&field4=%f&field5=%f&field6=%f&field7=%f&field8=%f\r\n"
            ,Motion_Out,Finger_Out,Amm_Out,Volt_Out,Light_Out,Thermo_Out,Temp_Out,Hum_Out);
    
    int i=0;
    for(i=0;ESP_8266_CMD_Send[i]!='\0';i++);
    i++;
    char cmd[255];
    
    sprintf(cmd,"AT+CIPSEND=%d",i);                                       //Send Number of open connection and Characters to send 
    ESP_8266_UART.SendCMD(cmd);
    if(FPS_Auth)
        FRDM_UART_Debug.printf("S\r\n%s",cmd);
    while(i<=20 || ESP_8266_CMD_Recv == ">")
    {
        ESP_8266_UART.RcvReply(ESP_8266_CMD_Recv, 1000);
        wait(100);
        i++;
    }
    //if(FPS_Auth)
        FRDM_UART_Debug.printf("R\r\n%s",ESP_8266_CMD_Recv);
    
    ESP_8266_UART.SendCMD(ESP_8266_CMD_Send);                                                      //Post value to thingspeak channel
    if(FPS_Auth)
        FRDM_UART_Debug.printf("S\r\n%s",ESP_8266_CMD_Send);
    
    while(i<=20 || ESP_8266_CMD_Recv == "OK")
    {
        ESP_8266_UART.RcvReply(ESP_8266_CMD_Recv, 1000);
        wait(100);
        i++;
    }
    //if(FPS_Auth)
        FRDM_UART_Debug.printf("R\r\n%s",ESP_8266_CMD_Recv);
    
}

