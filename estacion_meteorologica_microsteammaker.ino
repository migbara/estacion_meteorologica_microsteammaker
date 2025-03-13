#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <base64.h>
#include <Wire.h>
#include <math.h>
#include "AHT20.h"
#include "WiFi.h"

double n;
String s_function;
String s_red_wifi;
String s_endpoint;
String s_server;
String s_clave_wifi;
String s_frecuencia;
String s_endpoint_datetime;
String s_respuesta;
String s_valor;
String s_temperatura;
String s_humedad;
String s_fecha_hora;
String s_datos;
File sd_file;
StaticJsonDocument<1500> json_data;
char json_temp_data[1500];
DeserializationError json_error;
HTTPClient http_client;
AHT20 sensor_th_aht20;
unsigned long task_time_ms=0;

String espwifi_ssid="";
String espwifi_pass="";

void fnc_sd_print_esp32ks(String _f, String _t, boolean _nl){
	sd_file = SD.open(String("/")+_f, FILE_APPEND);
	if(sd_file){
		if(_nl)sd_file.println(_t);
		else sd_file.print(_t);
		sd_file.close();
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

double fnc_sensor_th_aht20(int _type){
  sensor_th_aht20.available();
  if(_type==1)return sensor_th_aht20.getTemperature();
  if(_type==2)return sensor_th_aht20.getHumidity();
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

void setup()
{


	Serial.begin(115200);
	Serial.flush();
	while(Serial.available()>0)Serial.read();

	SPIClass *spi_hspiSD = new SPIClass(HSPI); spi_hspiSD->begin(15, 32, 33, -1); SD.begin(-1, *spi_hspiSD, 40000000); //integrated SD card
	Wire.begin();
	sensor_th_aht20.begin();
	SD.begin(5); //integrated SD card micro:STEAMakers

	if (SD.exists(String("/")+String("app.log"))) {
		Serial.println(String("Existe el archivo app.log"));
	}
	else {
		Serial.println(String("No existe el archivo app.log"));
	}

	if (SD.exists(String("/")+String("config_red.txt"))) {
		s_red_wifi = fnc_sd_content_line_esp32ks(String("config_red.txt"),1);
		s_clave_wifi = fnc_sd_content_line_esp32ks(String("config_red.txt"),2);
		Serial.println(String("Datos de red recogidos"));
		Serial.println(s_red_wifi);
		Serial.println(s_clave_wifi);
		fnc_sd_print_esp32ks(String("app.log"),String(String("Datos de red recogidos")),true);
		fnc_sd_print_esp32ks(String("app.log"),String(s_red_wifi),true);
		fnc_sd_print_esp32ks(String("app.log"),String(s_clave_wifi),true);
		espwifi_setup(String(s_red_wifi),String(s_clave_wifi));
	}
	else {
		Serial.println(String("No se han podido leer datos de red"));
		fnc_sd_print_esp32ks(String("app.log"),String(String("No se han podido leer datos de red")),true);
	}

	if (SD.exists(String("/")+String("config_app.txt"))) {
		s_server = fnc_sd_content_line_esp32ks(String("config_app.txt"),1);
		s_frecuencia = fnc_sd_content_line_esp32ks(String("config_app.txt"),2);
		s_endpoint_datetime = fnc_sd_content_line_esp32ks(String("config_app.txt"),3);
		Serial.println(String("Datos de app recogidos"));
		Serial.println(s_server);
		Serial.println(s_frecuencia);
		Serial.println(s_endpoint_datetime);
		fnc_sd_print_esp32ks(String("app.log"),String(String("Datos de app recogidos")),true);
		fnc_sd_print_esp32ks(String("app.log"),String(s_server),true);
		fnc_sd_print_esp32ks(String("app.log"),String(s_frecuencia),true);
		fnc_sd_print_esp32ks(String("app.log"),String(s_endpoint_datetime),true);
	}
	else {
		Serial.println(String("No se han podido leer datos de app"));
		fnc_sd_print_esp32ks(String("app.log"),String(String("No se han podido leer datos de app")),true);
	}

	n = 10000;

}


void loop()
{
	yield();

	espwifi_check();

  	if((millis()-task_time_ms)>=n){
  		task_time_ms=millis();
  		if ((WiFi.status() == WL_CONNECTED)) {
  			s_function = String("/data/1");
  			Serial.println(String(s_server)+String(s_function));
  			s_endpoint = String(s_server)+String(s_function);
  			fnc_sd_print_esp32ks(String("app.log"),String(s_endpoint),true);
  			json_error = deserializeJson(json_data,String(fnc_http_client_get(s_endpoint,String(""),String(""),true,false)).c_str());
  			s_respuesta = fnc_http_client_get(s_endpoint,String(""),String(""),true,false);
  			Serial.println(s_respuesta);
  			fnc_sd_print_esp32ks(String("app.log"),String(s_respuesta),true);
  			if (json_data.containsKey(String("measure_type"))) {
  				s_valor = fnc_json_get_key_text(String("measure_type"));
  				Serial.println(s_valor);
  				fnc_sd_print_esp32ks(String("app.log"),String(s_valor),true);
  			}

  			s_function = String("/data");
  			s_endpoint = String(s_server)+String(s_function);
  			s_temperatura = String(fnc_sensor_th_aht20(1),2);
  			s_humedad = String(fnc_sensor_th_aht20(2),2);
  			json_error = deserializeJson(json_data,String(fnc_http_client_get(s_endpoint_datetime,String(""),String(""),true,false)).c_str());
  			s_respuesta = fnc_http_client_get(s_endpoint_datetime,String(""),String(""),true,false);
  			Serial.println(s_respuesta);
  			fnc_sd_print_esp32ks(String("app.log"),String(s_respuesta),true);
  			if (json_data.containsKey(String("datetime"))) {
  				s_fecha_hora = fnc_json_get_key_text(String("datetime"));
  				Serial.println(s_fecha_hora);
  				fnc_sd_print_esp32ks(String("app.log"),String(s_fecha_hora),true);
  				s_datos = String("{\"measure_type\": \"temperature\", \"date_time\": \"")+String(s_fecha_hora)+String("\", \"value\": ")+String(s_temperatura)+String(", \"unit\": \"C\"}");
  				Serial.println(s_datos);
  				fnc_sd_print_esp32ks(String("app.log"),String(s_datos),true);
  				s_respuesta = fnc_http_client_post(s_endpoint,s_datos,String(""),String(""),String("application/json"),true, false);
  				Serial.println(s_respuesta);
  				fnc_sd_print_esp32ks(String("app.log"),String(s_respuesta),true);
  				s_datos = String("{\"measure_type\": \"humidity\", \"date_time\": \"")+String(s_fecha_hora)+String("\", \"value\": ")+String(s_humedad)+String(", \"unit\": \"%\"}");
  				Serial.println(s_datos);
  				fnc_sd_print_esp32ks(String("app.log"),String(s_datos),true);
  				s_respuesta = fnc_http_client_post(s_endpoint,s_datos,String(""),String(""),String("application/json"),true, false);
  				Serial.println(s_respuesta);
  				fnc_sd_print_esp32ks(String("app.log"),String(s_respuesta),true);
  				n = String(s_frecuencia).toFloat();
  				fnc_sd_print_esp32ks(String("app.log"),String(String("Se volverá a leer en la frecuencia establecida")),true);
  			}
  			else {
  				fnc_sd_print_esp32ks(String("app.log"),String(String("No nos llega la hora. Hacemos que se ejecute en 1 minuto")),true);
  				n = 60000;
  			}

  		}
  		else {
  			Serial.println(String("No hay conexión Wifi"));
  			fnc_sd_print_esp32ks(String("app.log"),String(String("No hay conexión Wifi")),true);
  		}

  	}

}