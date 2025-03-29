#include <SPI.h>
#include <SD.h>
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Wire.h>
#include <math.h>
#include "AHT20.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <base64.h>

double n;
String s_mensaje;
String s_fichero;
String s_chatID;
String s_function;
String s_endpoint;
String s_temperatura;
String s_server;
String s_humedad;
String s_red_wifi;
String s_clave_wifi;
String s_telegramID;
String s_respuesta;
String s_endpoint_datetime;
String s_frecuencia;
String s_fecha_hora;
String s_datos;
File sd_file;
String espwifi_ssid="";
String espwifi_pass="";
WiFiClientSecure telegram_wifi_client;
UniversalTelegramBot telegram_bot("", telegram_wifi_client);
String telegram_message_text="";
String telegram_message_chatid="";
String telegram_message_fromname="";
String telegram_message_fromid="";
double telegram_message_lat=0.0;
double telegram_message_long=0.0;
unsigned long telegram_last_update;
AHT20 sensor_th_aht20;
StaticJsonDocument<1500> json_data;
char json_temp_data[1500];
DeserializationError json_error;
HTTPClient http_client;
unsigned long task_time_ms=0;

void fnc_sd_print_esp32ks(String _f, String _t, boolean _nl){
	sd_file = SD.open(String("/")+_f, FILE_APPEND);
	if(sd_file){
		if(_nl)sd_file.println(_t);
		else sd_file.print(_t);
		sd_file.close();
	}
}

void escribe_info() {
	Serial.println(s_mensaje);
	fnc_sd_print_esp32ks(s_fichero,String(s_mensaje),true);
	if ((!String(s_chatID).equals(String("")))) {
		telegram_bot.sendMessage(s_chatID,s_mensaje,"");
	}

}

String fnc_sd_content_line_esp32ks(String _f, int _line){
  int ln=0;
  char c;
  String cont="";
  bool enc=false;

  sd_file = SD.open(String("/")+_f, FILE_READ);

  if(sd_file){
    while(sd_file.available() && !enc){ //eof or found
      c=(char)sd_file.read();
      if(c!='\r'){  //discard
        if(c=='\n'){  //new line
          ln++;
          if(_line==ln){
            enc=true;
          }
          else{
            cont="";  //clear
          }
        }
        else{
          cont += c; //concat
        }
      }
    }
    sd_file.close();
  }
  return cont;
}

void espwifi_check(){
	if (WiFi.status() != WL_CONNECTED){
		WiFi.reconnect();
		delay(2000);
	}
}

void espwifi_setup(String _ssid, String _pass){
	WiFi.mode(WIFI_STA);
	espwifi_ssid=_ssid;
	espwifi_pass=_pass;
	WiFi.begin(_ssid.c_str(),_pass.c_str());
	while (WiFi.status() != WL_CONNECTED) delay(500);
}

double fnc_sensor_th_aht20(int _type){
  sensor_th_aht20.available();
  if(_type==1)return sensor_th_aht20.getTemperature();
  if(_type==2)return sensor_th_aht20.getHumidity();
}

void telegram_messages_getupdates(){
	if( (millis()-telegram_last_update)>=1000){
		int numNewMessages = telegram_bot.getUpdates(telegram_bot.last_message_received + 1);
		for (int i = 0; i < numNewMessages; i++){
			telegram_message_text=telegram_bot.messages[i].text;
			telegram_message_chatid=telegram_bot.messages[i].chat_id;
			telegram_message_fromname=telegram_bot.messages[i].from_name;
			telegram_message_fromid=telegram_bot.messages[i].from_id;

			telegram_message_lat=telegram_bot.messages[i].latitude;

			telegram_message_long=telegram_bot.messages[i].longitude;

	if (String(telegram_message_text).equals(String("/temperatura"))) {
		telegram_bot.sendMessage(s_chatID,(String(fnc_sensor_th_aht20(1),2)),"");
	}

	if (String(telegram_message_text).equals(String("/humedad"))) {
		telegram_bot.sendMessage(s_chatID,(String(fnc_sensor_th_aht20(2),2)),"");
	}


		}
		telegram_last_update=millis();
	}
}

String fnc_json_get_key_text(String _f){
	if(json_error)return "";
  JsonVariant t=json_data[_f];
  if( t.is<JsonObject>() || t.is<JsonArray>() || t.is<JsonObjectConst>() || t.is<JsonArrayConst>() ){
      serializeJson(t, json_temp_data);
      return String(json_temp_data);
  }
  return String(t.as<const char*>());
}

double fnc_json_get_key_number(String _f){
	if(json_error)return sqrt(-1);
  JsonVariant t=json_data[_f];
  if( (!t.is<double>()) && (!t.is<int>()) ){
      return 0;
  }
  return (double)(t.as<double>());
}

boolean fnc_json_get_key_bool(String _f){
	if(json_error)return false;
  JsonVariant t=json_data[_f];
  if(!t.is<boolean>() ){
      return 0;
  }
  return (boolean)(t.as<boolean>());
}

double fnc_json_get_array_size(String _f){
	if(json_error)return 0;
  if(_f=="")return json_data.size();
  JsonArray t=json_data[_f];
  if(t){
      return t.size();
  }
  return 0;
}

String  fnc_json_get_index_text(int _i){
	if(json_error)return "";
  JsonVariant t=json_data[_i];
  if( t.is<JsonObject>() || t.is<JsonArray>() || t.is<JsonObjectConst>() || t.is<JsonArrayConst>() ){
      serializeJson(t, json_temp_data);
      return String(json_temp_data);
  }
  return String(t.as<const char*>());
}

double fnc_json_get_index_number(int _i){
	if(json_error)return sqrt(-1);
  JsonVariant t=json_data[_i];
  if( (!t.is<double>()) && (!t.is<int>()) ){
      return sqrt(-1);
  }
  return (double)(t.as<double>());
}

boolean fnc_json_get_index_bool(int _i){
	if(json_error)return false;
  JsonVariant t=json_data[_i];
  if( !t.is<boolean>() ){
      return false;
  }
  return (boolean)(t.as<boolean>());
}

String fnc_http_client_get(String _url, String _user, String _pass,bool _response, bool _redirect){
	String resp="";
	int respcode;
	if(_redirect){
		http_client.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
	}
	else{
		http_client.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
	}
	http_client.begin(_url.c_str());
	if(_user!="" || _pass!=""){
		String auth = base64::encode(_user + ":" + _pass);
		http_client.addHeader("Authorization", "Basic " + auth);
	}
	respcode=http_client.GET();
	if(_response){
		if(respcode>0){
			resp=http_client.getString();
		}else{
			resp="HTTP-GET-ERROR: "+String(respcode);
		}
	}
	http_client.end();
	return resp;
}

String fnc_http_client_post(String _url, String _data, String _user, String _pass, String _contenttype, bool _response, bool _redirect){
	String resp="";
	int respcode;
	if(_redirect){
		http_client.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
	}
	else{
		http_client.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
	}
	http_client.begin(_url.c_str());
	if(_user!="" || _pass!=""){
		String auth = base64::encode(_user + ":" + _pass);
		http_client.addHeader("Authorization", "Basic " + auth);
	}
	http_client.addHeader("Content-Type",_contenttype);
	respcode=http_client.POST(_data);
	if(_response){
		if(respcode>0){
			resp=http_client.getString();
		}else{
			resp="HTTP-POST-ERROR: "+String(respcode);
		}
	}
	return resp;
}

void setup()
{


	Serial.begin(115200);
	Serial.flush();
	while(Serial.available()>0)Serial.read();

	SPIClass *spi_hspiSD = new SPIClass(HSPI); spi_hspiSD->begin(15, 32, 33, -1); SD.begin(-1, *spi_hspiSD, 40000000); //integrated SD card
	SD.begin(5); //integrated SD card micro:STEAMakers
	Wire.begin();
	sensor_th_aht20.begin();

	n = 10000;
	s_chatID = String("");
	s_fichero = String("app.log");
	if (SD.exists(String("/")+String("config_red.txt"))) {
		s_red_wifi = fnc_sd_content_line_esp32ks(String("config_red.txt"),1);
		s_clave_wifi = fnc_sd_content_line_esp32ks(String("config_red.txt"),2);
		s_mensaje = String("Datos de red recogidos")+String("\n")+String(s_red_wifi)+String("\n")+String(s_clave_wifi);
		escribe_info();
		espwifi_setup(String(s_red_wifi),String(s_clave_wifi));
	}
	else {
		s_mensaje = String("No se han podido leer datos de red");
		escribe_info();
	}

	if (SD.exists(String("/")+String("config_telegram.txt"))) {
		s_telegramID = fnc_sd_content_line_esp32ks(String("config_telegram.txt"),1);
		s_chatID = fnc_sd_content_line_esp32ks(String("config_telegram.txt"),2);
		telegram_wifi_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
		telegram_bot.updateToken(String(s_telegramID));
		telegram_bot.sendMessage(s_chatID,String("Iniciando app en ESP32 Micro steammaker"),"");
		s_mensaje = String("Datos de telegram recogidos");
		escribe_info();
		if (SD.exists(String("/")+String("app.log"))) {
			Serial.println(String("Existe el archivo app.log"));
		}
		else {
			Serial.println(String("No existe el archivo app.log"));
		}

	}

	if (SD.exists(String("/")+String("config_app.txt"))) {
		s_server = fnc_sd_content_line_esp32ks(String("config_app.txt"),1);
		s_frecuencia = fnc_sd_content_line_esp32ks(String("config_app.txt"),2);
		s_endpoint_datetime = fnc_sd_content_line_esp32ks(String("config_app.txt"),3);
		s_mensaje = String("Datos de app recogidos")+String("\n")+String(s_server)+String("\n")+String(s_frecuencia)+String("\n")+String(s_endpoint_datetime);
		escribe_info();
	}
	else {
		s_mensaje = String("No se han podido leer datos de app");
		escribe_info();
	}


}


void loop()
{
	yield();

	espwifi_check();

	telegram_messages_getupdates();
  	if((millis()-task_time_ms)>=n){
  		task_time_ms=millis();
  		if ((WiFi.status() == WL_CONNECTED)) {
  			s_function = String("/data");
  			s_endpoint = String(s_server)+String(s_function);
  			s_temperatura = String(fnc_sensor_th_aht20(1),2);
  			s_humedad = String(fnc_sensor_th_aht20(2),2);
  			json_error = deserializeJson(json_data,String(fnc_http_client_get(s_endpoint_datetime,String(""),String(""),true,false)).c_str());
  			s_respuesta = fnc_http_client_get(s_endpoint_datetime,String(""),String(""),true,false);
  			s_mensaje = s_respuesta;
  			escribe_info();
  			if (json_data.containsKey(String("datetime"))) {
  				s_fecha_hora = fnc_json_get_key_text(String("datetime"));
  				s_mensaje = s_fecha_hora;
  				escribe_info();
  				s_datos = String("{\"measure_type\": \"temperature\", \"date_time\": \"")+String(s_fecha_hora)+String("\", \"value\": ")+String(s_temperatura)+String(", \"unit\": \"C\"}");
  				s_mensaje = s_datos;
  				escribe_info();
  				s_respuesta = fnc_http_client_post(s_endpoint,s_datos,String(""),String(""),String("application/json"),true, false);
  				s_mensaje = s_respuesta;
  				escribe_info();
  				s_datos = String("{\"measure_type\": \"humidity\", \"date_time\": \"")+String(s_fecha_hora)+String("\", \"value\": ")+String(s_humedad)+String(", \"unit\": \"%\"}");
  				s_mensaje = s_datos;
  				escribe_info();
  				s_respuesta = fnc_http_client_post(s_endpoint,s_datos,String(""),String(""),String("application/json"),true, false);
  				s_mensaje = s_respuesta;
  				escribe_info();
  				n = String(s_frecuencia).toFloat();
  				s_mensaje = String("Preceso realizado. Se volverá a realizar en la frecuencia establecida");
  				escribe_info();
  			}
  			else {
  				n = 10000;
  				s_mensaje = String("No nos llega la hora. Hacemos que se ejecute de nuevo");
  				escribe_info();
  			}

  		}
  		else {
  			s_mensaje = String("No hay conexión Wifi");
  			escribe_info();
  		}

  	}

}