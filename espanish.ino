#include <WiFi.h>
#include <WebServer.h>
#include <Stepper.h>

#define IN1 32
#define IN2 33
#define IN3 25
#define IN4 26

#define RELE1 14
#define RELE2 12

#define PIN_LED 4
#define PIN_BOTON 27  

WebServer servidor(80);
Stepper motorAPasos(2048, IN1, IN2, IN3, IN4);

int brilloLED = 0;
int pasosRestantes = 0;
int pasosRealizados = 0;

const char* redSSID = "UPPue-WiFi";
const char* contrasena = "";
IPAddress ipEstatica(192, 168, 1, 184);
IPAddress puertaEnlace(192, 168, 1, 1);
IPAddress mascaraSubred(255, 255, 255, 0);

bool usarIPEstatica = false;

unsigned long ultimoTiempoPaso = 0;

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_BOTON, INPUT_PULLUP); /
  
  verificarEstadoBoton(); 
  configurarWiFi();

  pinMode(RELE1, OUTPUT);
  pinMode(RELE2, OUTPUT);
  digitalWrite(RELE1, LOW);
  digitalWrite(RELE2, LOW);

  motorAPasos.setSpeed(15);
  pinMode(PIN_LED, OUTPUT);
  
  servidor.on("/", HTTP_GET, manejarRaiz);
  servidor.on("/motor_on", HTTP_GET, manejarMotorEncendido);
  servidor.on("/motor_off", HTTP_GET, manejarMotorApagado);
  servidor.on("/set_steps", HTTP_GET, manejarConfigurarPasos);
  servidor.on("/set_pwm", HTTP_GET, manejarConfigurarPWM);
  servidor.on("/progress_update", HTTP_GET, manejarActualizarProgreso);
  servidor.begin();
}

void loop() {
  servidor.handleClient();
  
  static bool estadoAnteriorBoton = HIGH;
  bool estadoActualBoton = digitalRead(PIN_BOTON);

  if (estadoActualBoton != estadoAnteriorBoton) {
    delay(50); // Debouncing
    if (estadoActualBoton != estadoAnteriorBoton) {
      estadoAnteriorBoton = estadoActualBoton;
      verificarEstadoBoton();
      configurarWiFi();
    }
  }

  // Actualización del motor a pasos
  if (pasosRealizados < pasosRestantes) {
    unsigned long tiempoActual = millis();
    if (tiempoActual - ultimoTiempoPaso >= 10) { 
      motorAPasos.step(1);
      pasosRealizados++;
      ultimoTiempoPaso = tiempoActual;
    }
  }
}

void verificarEstadoBoton() {
  usarIPEstatica = (digitalRead(PIN_BOTON) == HIGH); // 1 = IP estática, 0 = IP dinámica
}

void configurarWiFi() {
  WiFi.disconnect(); 
  delay(100); 
  
  if (usarIPEstatica) {
    WiFi.config(ipEstatica, puertaEnlace, mascaraSubred);
    Serial.println("Configurando con IP estática...");
  } else {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); 
    Serial.println("Configurando con IP dinámica...");
  }

  WiFi.begin(redSSID, contrasena);

  unsigned long tiempoInicioIntento = millis();
  const unsigned long tiempoLimite = 30000; 

  while (WiFi.status() != WL_CONNECTED && millis() - tiempoInicioIntento < tiempoLimite) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConexión exitosa.");
    Serial.print("IP asignada: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nError: No se pudo conectar a la red WiFi.");
  }
}

void manejarRaiz() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<style>";
  html += "body { text-align: center; margin: 0; font-family: Arial, sans-serif; }";
  html += ".header { display: flex; justify-content: space-between; align-items: center; padding: 10px; }";
  html += ".header img { height: 77px; }";
  html += ".content { padding: 20px; }";
  html += "</style>";
  html += "</head><body>";

  html += "<div class='header'>";
  html += "<img src='https://www.archimundo.com/img/imagenes_instituciones/mx/1620-UPPue/logo.png' alt='Logo UPP'>";
  html += "<h1 style='flex-grow: 1; margin: 0;'>Universidad Politecnica de Puebla</h1>";
  html += "<img src='https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcQeJz3-7Fn0y049BNdt1QuTuc0daMXRK1_4hg&amp;s' alt='Logo Mecatronica'>";
  html += "</div>";

  html += "<div class='content'>";
  html += "<h2>Programacion de perifericos</h2>";
  html += "<h3>Control del motor de potencia</h3>";
  
  html += "<div style='margin-bottom: 20px;'>";
  html += "<button onclick=\"window.location.href='/motor_on'\" style='background-color: green; color: white; font-size: 20px; padding: 15px 30px;'>Motor On</button>";
  html += "<button onclick=\"window.location.href='/motor_off'\" style='background-color: red; color: white; font-size: 20px; padding: 15px 30px; margin-left: 20px;'>Motor Off</button>";
  html += "</div>";

  html += "<div style='display: flex; justify-content: space-between;'>";
  
  html += "<div style='text-align: left; width: 45%;'>";
  html += "<h3>Control de LED con PWM</h3>";
  html += "<input type='range' id='led_pwm' min='0' max='255' value='" + String(brilloLED) + "' style='width: 100%; height: 35px;' onchange='setPWM()'><br>";
  html += "<h4>Porcentaje: <span id='led_value'>" + String(map(brilloLED, 0, 255, 0, 100)) + "%</span></h4>";
  html += "</div>";

  html += "<div style='text-align: right; width: 45%;'>";
  html += "<h3>Control del motor a pasos</h3>";
  html += "<input type='number' id='steps' min='0' max='1000' value='0'> <button onclick='setSteps()'>Set Steps</button><br>";
  html += "<progress id='progress_bar' value='" + String(pasosRealizados) + "' max='1000' style='width: 100%; height: 25px;'></progress>";
  html += "<h4>Pasos realizados: <span id='steps_done'>" + String(pasosRealizados) + "</span> de 1000</h4>";
  html += "</div>";

  html += "</div>"; 

  html += "<script>";
  html += "function setSteps() { var steps = document.getElementById('steps').value; window.location.href = '/set_steps?steps=' + steps; }";
  html += "function setPWM() { var pwm = document.getElementById('led_pwm').value; window.location.href = '/set_pwm?pwm=' + pwm; document.getElementById('led_value').innerText = Math.round(pwm / 255 * 100) + '%'; }";
  html += "setInterval(() => { fetch('/progress_update').then(response => response.json()).then(data => { document.getElementById('progress_bar').value = data.stepsCompleted; document.getElementById('steps_done').innerText = data.stepsCompleted; }); }, 500);";
  html += "</script>";

  html += "</body></html>";

  servidor.send(200, "text/html", html);
}

void manejarMotorEncendido() {
  digitalWrite(RELE1, HIGH);
  delay(1000);
  digitalWrite(RELE1, LOW);
  manejarRaiz();
}

void manejarMotorApagado() {
  digitalWrite(RELE2, HIGH);
  delay(1000);
  digitalWrite(RELE2, LOW);
  manejarRaiz();
}

void manejarConfigurarPasos() {
  if (servidor.hasArg("steps")) {
    pasosRestantes = servidor.arg("steps").toInt();
    pasosRealizados = 0;
    ultimoTiempoPaso = millis();
  }
  manejarRaiz();
}

void manejarConfigurarPWM() {
  if (servidor.hasArg("pwm")) {
    brilloLED = servidor.arg("pwm").toInt();
    analogWrite(PIN_LED, brilloLED);
  }
  manejarRaiz();
}

void manejarActualizarProgreso() {
  String json = "{\"stepsCompleted\": " + String(pasosRealizados) + "}";
  servidor.send(200, "application/json", json);
}
