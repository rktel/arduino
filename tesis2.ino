unsigned int tstart;
#define RESTART asm("jmp 0x0000")

#include <SoftwareSerial.h>
SoftwareSerial camSerial(50, 51); //para arduino UNO (2,3); para MEGA (50,51)
SoftwareSerial gpsSerial(10, 11); //para arduino UNO (4,5); para MEGA (10,11)
SoftwareSerial gsmSerial(52, 53); //para arduino UNO (7,8); para MEGA (52,53)

//VARIABLES PARA CAMARA
byte incomingbyte;
int a=0x0000,j=0,k=0,count=0; //Read Starting address 
uint8_t MH,ML;
boolean EndFlag=0;
byte cam[34];
int indice=0;

//VARIABLES PARA MODULO GSM
int8_t answer;
const int pinOnGsm= 9;

//VARIABLE GPS
char readSerial[49];
byte contadorGps = 0;

//CONFIGURACION DE SENSOR MAGNETICO Y LED
const int buttonPin = 12; // el número del pin, entrada del pulsador
const int ledPin = 13;
int buttonState = 0; // estado actual del pulsador
int lastButtonState = 0; // estado anterior del pulsador

void setup(){
 
 //INICIANDO PUERTO SERIAL
 Serial.begin(9600);
 camSerial.begin(38400);
 gpsSerial.begin(9600);
 gsmSerial.begin(9600);
 
 //CONFIGURANDO MODULO GPS
 setupGps();
 
 //CONFIGURANDO PINES GSM //RESET y CONEXION UDP MODULO GSM
 pinMode(pinOnGsm,OUTPUT);
 digitalWrite(pinOnGsm,LOW);
 powerGsm();
 connGsm();
 
 //INICIANDO SENSOR MAGNETICO Y LED13
 pinMode(ledPin, OUTPUT);
 pinMode(buttonPin, INPUT);
 digitalWrite(buttonPin,HIGH);
}


void loop() 
{ gps();
 statusButton();
 gsmSerialAvailable();
}

//ESTADO DE BOTON
void statusButton(){
 buttonState = digitalRead(buttonPin);
 delay(1);
 if (buttonState != lastButtonState){
 if (buttonState == LOW) {
 Serial.println(">>> MODO CAPTURA IMAGEN <<<");EndFlag=0; coberturaGsm();
 SendResetCmd(); sendPhoto();
 }
 }
 lastButtonState = buttonState;
}

void gsmSerialAvailable(){
 gsmSerial.listen();
 if(Serial.available()>0){
 char caracter1 = Serial.read();
 gsmSerial.print(caracter1);
 }
 
 if(gsmSerial.available()>0){
 char caracter2 = gsmSerial.read();
 Serial.print(caracter2);
 if( caracter2 == 'c'){
 Serial.println(">>> MODO CAPTURA IMAGEN <<<");EndFlag=0; coberturaGsm();
 SendResetCmd(); sendPhoto();
 }
 }
}

//------- COBERTURA GSM --------------------------
void coberturaGsm(){
 byte indicador = 0;
 gsmSerial.listen();
 if (sendATcommand("AT+CIPSTATUS", "CONNECT", 500) == 1){ Serial.println("Conectado"); }
 else{
 
 while (sendATcommand("AT+CIPSHUT", "OK", 1000) != 1){
 delay(1000);
 Serial.println("Restableciendo conexion...");
 }
 while (sendATcommand("AT+CIPSTART=\"UDP\",\"10.40.0.20\",\"8000\"", "CONNECT OK", 3000) != 1){
 delay(1000);
 Serial.println("conectando...");
 }
 Serial.println("conectado");
 }
}

//--------ESCRITURA Y ENVIO DE IMAGEN-------------------
void sendPhoto(){
 camSerial.listen();
 SendTakePhotoCmd();delay(1000); Serial.println("Adquiriendo datos de imagen..."); 
 while(camSerial.available()>0) { incomingbyte=camSerial.read(); } 
 
 while(!EndFlag){
 j=2; k=0; count=2; SendReadDataCmd(); delay(25);memset(cam,0,34);
 
 if (indice<=255){cam[0]=0; cam[1]=indice;}
 else {cam[0]=(indice>>8); cam[1]=indice;}
 delay(10);
 
 while(camSerial.available()>0){
 incomingbyte=camSerial.read(); k++;
 if((k>5)&&(j<34)&&(!EndFlag)){
 cam[j]=incomingbyte;
 if((cam[j-1]==0xFF)&&(cam[j]==0xD9)) EndFlag=1; //Check if the picture is over
 j++; count++;
 }
 }
 indice++;
 delay(50);
 
 digitalWrite(ledPin, LOW);
 gsmSerial.listen();
 if (sendATcommandgsm("AT+CIPSEND=34", ">", "ERROR", 2000) == 1){gsmSerial.write(cam, 68);}
 digitalWrite(ledPin, HIGH);
 
 camSerial.listen();
 for(j=0;j<count;j++){
 if(cam[j]<0x10){ Serial.print("0");}
 Serial.print(cam[j],HEX);
 if((cam[j-1]==0xFF)&&(cam[j]==0xD9)) {Serial.println();Serial.println("IMAGEN CREADA"); indice=0;}
 }
 Serial.println();
 }
 a=0x0000;
}

//-----------CAPTURA E IMPRESION GPS---------------------
void gps(){
 gpsSerial.listen();
 
 
 char readByte;
 char tramaGPS[50];
 memset(tramaGPS,0,50);
 int i = 0;
 
 delay(100);
 
 if (gpsSerial.available()>0){
 while (gpsSerial.available()>0){
 tramaGPS[i]= gpsSerial.read();
 i++;
 //Serial.print(tramaGPS[i]);
 //readByte = gpsSerial.read();
 
 }
 Serial.print(tramaGPS);
 //delay(100);
 }
 
 /*byte inByte = '\0';
 while(inByte!='$'){inByte=gpsSerial.read();}
 
 if(inByte=='$'){
 
 while(gpsSerial.available()<50){;}
 for(int i=1;i<51;i++){readSerial[i]= gpsSerial.read();}
 
 }
 
 readSerial[0]='$'; readSerial[49]='\0';
 
 gpsWriteGsm();
 Serial.println(readSerial);
 contadorGps = 0;
 memset(readSerial,'\0',49); 
 */
}

//------------CONFIGURACION MODULO GPS-------------- 
void setupGps(){
 gpsSerial.listen();
 delay(500);
 gpsSerial.println("$PMTK314,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29");
 delay(100);
 gpsSerial.println("$PMTK300,10000,0,0,0,0*2C");
 Serial.println("INICIO DE MODULO GPS OK");
}

//--------------ENCENDIDO O RESET MODULO GSM
void powerGsm(){
 gsmSerial.listen();
 uint8_t answer1=0,answer2=0;
 
 
 answer1 = sendATcommand("AT+CPOWD=1", "NORMAL", 5000);
 if(answer1 == 1){
   digitalWrite(pinOnGsm,LOW); delay(5000);
   digitalWrite(pinOnGsm,HIGH); delay(3500);
   digitalWrite(pinOnGsm,LOW);
 }
 else{
   digitalWrite(pinOnGsm,LOW); delay(500);
   digitalWrite(pinOnGsm,HIGH); delay(3500);
   digitalWrite(pinOnGsm,LOW);
 }
 delay(7000);
 answer2 = sendATcommand("AT", "OK", 2000);
 while(answer2 == 0){ answer2 = sendATcommand("AT", "OK", 2000); }
 delay(7000);
 
 Serial.println("ENCENDIDO DE MODULO GSM OK");
}


//-------------COMANDO PARA ENVIAR DATOS GPS MODULO GSM--------------------
void gpsWriteGsm(){
 gsmSerial.listen();
 gsmSerial.println("AT+CIPSEND=49"); 
 delay(150);
 gsmSerial.print(readSerial); 
 }

//------------CREANDO SOCKET UDP MODULO GSM-------------------
void connGsm(){

 
 while (sendATcommand("AT+CSTT=\"MOVISTAR.PE\"", "OK", 3000) != 1){
 Serial.println("ERROR APN");
 
 }

 while (sendATcommand("AT+CIICR", "OK", 4000) != 1){
 Serial.println("ERROR CONEXION WIRELESS");
 }
 
 while (sendATcommand("AT+CIFSR", ".", 1000) != 1){
 Serial.println("ERROR IP");
 }
 
 while (sendATcommand("AT+CIPSTART=\"UDP\",\"190.234.198.127\",\"8000\"", "CONNECT OK", 1000) != 1){
 Serial.println("ERROR AL ABRIR SOCKET");
 }
 
 Serial.println("INICIO DE MODULO GSM OK\n");
 Serial.println("MODULO CONECTADO");
 delay(2000);
 digitalWrite(ledPin, HIGH);
 
}
 
//------------COMANDO MODULO GSM----------------
int8_t sendATcommand(char* ATcommand, char* expected_answer1, unsigned int timeout){

 uint8_t x=0, answer=0; char response[100]; unsigned long previous;
 memset(response, '\0', 100); delay(100);
 
 gsmSerial.listen();
 while( gsmSerial.available() > 0) gsmSerial.read(); 
 gsmSerial.println(ATcommand); x = 0; previous = millis();

 // this loop waits for the answer
 do{
 if(gsmSerial.available() != 0){ 
 response[x] = gsmSerial.read(); x++;
 if (strstr(response, expected_answer1) != NULL) { answer = 1; }
 }
 } while((answer == 0) && ((millis() - previous) < timeout)); 
 return answer;
}

int8_t sendATcommandgsm(char* ATcommand, char* expected_answer1, char* expected_answer2, unsigned int timeout){

 uint8_t x=0, answer=0;
 char response[100];
 unsigned long previous;
 memset(response, '\0', 100); // Initialize the string
 delay(100);
 
 while( gsmSerial.available() > 0) gsmSerial.read(); // Clean the input buffer

 gsmSerial.println(ATcommand); // Send the AT command 
 x = 0;
 previous = millis();

 // this loop waits for the answer
 do{
 // if there are data in the UART input buffer, reads it and checks for the asnwer
 if(gsmSerial.available() != 0){ 
 response[x] = gsmSerial.read();
 x++;
 // check if the desired answer 1 is in the response of the module
 if (strstr(response, expected_answer1) != NULL) 
 {
 answer = 1;
 }
 // check if the desired answer 2 is in the response of the module
 else if (strstr(response, expected_answer2) != NULL) 
 {
 answer = 2;
 }
 }
 }
 // Waits for the asnwer with time out
 while((answer == 0) && ((millis() - previous) < timeout)); 

 return answer;
}

//--------------COMANDOS DE CONTROL DE LA CAMARA-------------------------------
//Send Reset command
void SendResetCmd()
{
 camSerial.listen();
 Serial.println("INICIANDO CAMARA");
 camSerial.write(0x56);
 camSerial.write(byte(0x00));
 camSerial.write(0x26);
 camSerial.write(byte(0x00));
 delay(4000);
}

//Send take picture command
void SendTakePhotoCmd()
{
 camSerial.write(0x56);
 camSerial.write(byte(0x00));
 camSerial.write(0x36);
 camSerial.write(0x01);
 camSerial.write(byte(0x00)); 
}

//Read data
void SendReadDataCmd()
{
 MH=a/0x100;
 ML=a%0x100;
 camSerial.write(0x56);
 camSerial.write(byte(0x00));
 camSerial.write(0x32);
 camSerial.write(0x0c);
 camSerial.write(byte(0x00)); 
 camSerial.write(0x0a);
 camSerial.write(byte(0x00));
 camSerial.write(byte(0x00));
 camSerial.write(MH);
 camSerial.write(ML); 
 camSerial.write(byte(0x00));
 camSerial.write(byte(0x00));
 camSerial.write(byte(0x00));
 camSerial.write(0x20);
 camSerial.write(byte(0x00)); 
 camSerial.write(0x0a);
 a+=0x20; //address increases 32£¬set according to buffer size
}

void StopTakePhotoCmd()
{
 camSerial.write(0x56);
 camSerial.write(byte(0x00));
 camSerial.write(0x36);
 camSerial.write(0x01);
 camSerial.write(0x03); 
}
