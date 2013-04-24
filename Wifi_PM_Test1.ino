
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define Device_yeelink          2321
#define Sensor_PM1_yeelink       3213
#define Sensor_PM25_yeelink       3248 
#define API_Key_yeelink         "efffc62bc552e0fda14ac7d6b28fb530"         

int pin = 10;
unsigned long duration;
unsigned long duration2;
unsigned long starttime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
unsigned long lowpulseoccupancy2 = 0;
float ratio = 0;
float concentration = 0;
float ratio2 = 0;
float concentration2 = 0;

float average_con = 0.0;
float average_con2 = 0.0;
float con[120];
float con2[120];
int flag = -119;
int flag2 = -119;

float sum=0.0;
float sum2=0.0;

int temp = 0;

LiquidCrystal_I2C lcd(0x20,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display

void setup() {
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  pinMode(10,INPUT);
  pinMode(7,INPUT);
  starttime = millis();
  
  //Initialize LCD
  lcd.init();
  lcd.setCursor(0,1);
  lcd.print("PM1.0:    , POOR");
}

void loop() {
  duration = pulseIn(10, LOW);
  duration2 = pulseIn(7, LOW);
  lowpulseoccupancy = lowpulseoccupancy+duration;
  lowpulseoccupancy2 = lowpulseoccupancy2+duration2;
  
  if ((millis()-starttime) > sampletime_ms)
  {
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
    ratio2 = lowpulseoccupancy2/(sampletime_ms*10.0);  // Integer percentage 0=>100
    concentration2 = 1.1*pow(ratio2,3)-3.8*pow(ratio2,2)+520*ratio2+0.62; // using spec sheet curve

    average_con = low_pass_average_hour(con, &flag, concentration, &sum);
    average_con2 = low_pass_average_hour(con2, &flag2, concentration2, &sum2);

    lowpulseoccupancy = 0;
    lowpulseoccupancy2 = 0;
    wifi_update_yeelink(average_con,Device_yeelink,Sensor_PM1_yeelink,API_Key_yeelink);
    LCD_Print(average_con);
    starttime = millis();
    while((millis()-starttime) < 6000)
    {
      temp++;
    }
    temp = 0;
    wifi_update_yeelink(average_con2,Device_yeelink,Sensor_PM25_yeelink,API_Key_yeelink);

    starttime = millis();
  }
}

float low_pass_average_hour(float* con, int* flag, float concentration,float* sum)
{
    float average = 0.0;
    if( (*flag) < 0)
    {
        con[119+(*flag)] = concentration;
        (*sum) = (*sum) + concentration;
        (*flag) += 1;
        average = (*sum)/((*flag)+119);

        return average;      
    }
    else
    {
        *sum = *sum - con[(*flag)];
        con[(*flag)] = concentration;
        *sum = *sum + concentration;
        *flag = *flag + 1;
        if(*flag > 119)
        {
            (*flag) = 0;
        }
        average = (*sum)/120;
        return average;
    }
}

void wifi_update_yeelink(float concentration,int device,int sensor,char* api_key)
{
    Serial.print("POST /v1.0/device/");
    Serial.print(device);
    Serial.print("/sensor/");

    Serial.print(sensor);

    Serial.print("/datapoints");
    Serial.println(" HTTP/1.1");
    Serial.println("Host: api.yeelink.net");
    Serial.print("Accept: *");
    Serial.print("/");
    Serial.println("*");
    Serial.print("U-ApiKey: ");
    Serial.println(API_Key_yeelink);
    Serial.print("Content-Length: ");

    // calculate the length of the sensor reading in bytes:
    // 8 bytes for {"value":} + number of digits of the data:
    int thisLength = 10 + getLength(concentration);
    Serial.println(thisLength);

    Serial.println("Content-Type: application/x-www-form-urlencoded");
    Serial.println("Connection: close");
    Serial.println();

    // here's the actual content of the PUT request:
    Serial.print("{\"value\":");
    Serial.print(concentration);
    Serial.println("}");
}

int getLength(float num)
{
  int i=0;
  num = num*100;
  while(num>=10)
  {
    num/=10;
    i++;
  }
  i=i+2;
  return i;
}

void LCD_Print(float PM)
{
  lcd.setCursor(6,1);
  if(PM<1000)
   {
     lcd.print(" ");
   }
  lcd.print((int)PM);
  
  lcd.setCursor(11,1);
  if(PM>0 && PM<75)
  {
    lcd.print("VGood");
    lcd.noBacklight();
  }
  else if(PM<150)
  {
    lcd.print(" Good");
    lcd.noBacklight();
  }
  else if(PM<300)
  {
    lcd.print("   OK");
    lcd.noBacklight();
  }
  else if(PM<1050)
  {
    lcd.print(" Poor");
    lcd.noBacklight();
  }
  else
  {
    lcd.print("VPoor");
    lcd.backlight();
  }
}
