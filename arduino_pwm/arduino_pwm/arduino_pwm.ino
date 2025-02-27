const int motorEsquerdoPin1 = 5;
const int motorEsquerdoPin2 = 6;
const int motorDireitoPin1 = 9;
const int motorDireitoPin2 = 10;

String inputString = "";
boolean stringComplete = false;

void setup() {
  pinMode(motorEsquerdoPin1, OUTPUT);
  pinMode(motorEsquerdoPin2, OUTPUT);
  pinMode(motorDireitoPin1, OUTPUT);
  pinMode(motorDireitoPin2, OUTPUT);
  
  Serial.begin(115200);
  inputString.reserve(200);

  Serial.print("Iniciado sistema \n");
}

void move(int left_power, int right_power) {
  left_power = constrain(left_power, -100, 100);
  right_power = constrain(right_power, -100, 100);
  
  int leftPWM = map(left_power, -100, 100, -255, 255);
  int rightPWM = map(right_power, -100, 100, -255, 255);
  
  if (leftPWM > 0) {
    analogWrite(motorEsquerdoPin1, leftPWM);
    analogWrite(motorEsquerdoPin2, 0);
  } else if (leftPWM < 0) {
    analogWrite(motorEsquerdoPin1, 0);
    analogWrite(motorEsquerdoPin2, abs(leftPWM));
  } else {
    analogWrite(motorEsquerdoPin1, 0);
    analogWrite(motorEsquerdoPin2, 0);
  }
  
  if (rightPWM > 0) {
    analogWrite(motorDireitoPin1, rightPWM);
    analogWrite(motorDireitoPin2, 0);
  } else if (rightPWM < 0) {
    analogWrite(motorDireitoPin1, 0);
    analogWrite(motorDireitoPin2, abs(rightPWM));
  } else {
    analogWrite(motorDireitoPin1, 0);
    analogWrite(motorDireitoPin2, 0);
  }


}

void loop() {
  if (stringComplete) {
    // Verifica se o comando começa com "power:"
    Serial.print("[ARDUINO]: Received: "+ inputString);
    if (inputString.startsWith("power:")) {
      int firstComma = inputString.indexOf(':');
      int secondComma = inputString.indexOf(',', firstComma + 1);          

      if (firstComma != -1 && secondComma != -1) {
        int left_power = inputString.substring(firstComma + 1, secondComma).toInt();
        int right_power = inputString.substring(secondComma + 1).toInt();
          
        //char buffer[30];
        //snprintf(buffer, sizeof(buffer), "inputed: L: %d, R: %d\n", left_power, right_power);
        //Serial.print(buffer);

        move(left_power, right_power);
      }
    }
    
    inputString = "";
    stringComplete = false;
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}