import processing.serial.*;

int lf = 10;    // Linefeed in ASCII
String myString = null;
String[] list;
int[] value= new int[15];
Serial myPort;  // The serial port
boolean beatstat=false;


void setup() {
  size(1000, 1000);
  background(0);
  // List all the available serial ports
  printArray(Serial.list());
  // Open the port you are using at the rate you want:
  myPort = new Serial(this, Serial.list()[4], 9600);
  myPort.clear();
  // Throw out the first reading, in case we started reading 
  // in the middle of a string from the sender.
  myString = myPort.readStringUntil(lf);
  myString = null;


  for (int i=0; i<10; i++) {
    value[i]=0;
  }
}

void draw() {
  while (myPort.available() > 0) {
    myString = myPort.readStringUntil(lf);
    if (myString != null) {
      list = split(myString, ',');
      for (int i=0; i<list.length-1; i++) {
        value[i]=int(list[i]);
      }
    }
  }
  background(0);
  fill(255);
  textAlign(RIGHT);
  textSize(20);

  for (int i=0; i<10; i+=2) {
    text(value[i], 150+80*i, 40);
    rect(100+80*i, 70, 80, 10+value[i]);
  } 
  for (int i=1; i<10; i+=2) {
    fill(0,255,0,100);
    text(value[i], 70+80*i, 60);
    rect(20+80*i, 70, 80, 10+value[i]);
  }
  for (int i=0; i<5; i++) {
    
    fill(value[i*2]/2,i*255/5,255);
    rect(100+160*i, height-180, 80, 80);
  } 
}
