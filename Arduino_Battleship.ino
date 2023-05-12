/*
  REFERENCES:

    Igoe, T. (2023, February 20). Control an 8x8 matrix of Leds: Arduino documentation. Retrieved March 5, 2023, from 
    https://docs.arduino.cc/built-in-examples/display/RowColumnScanning 

    SAnwandter1. (2017, February 17). Programming 8x8 LED Matrix. Retrieved from 
    https://projecthub.arduino.cc/SAnwandter1/a3b85262-a762-417a-ac28-f91231349887 

*/

#include <LedControl.h> //define ledcontrol library for 8x8 matrix

//matrix pins
int DIN = 12;
int CS =  11;
int CLK = 10;

// initialize matrix
LedControl matrix = LedControl(DIN,CLK,CS,1);

//led pins
int greenLed = 6;
int redLed = 5;

//button pins
int orientButton = 4;
int selectButton = 3;

//initialize number of ships
int ships;

//this subsystem will start first so myTurn will start as true.
bool myTurn = true;

//struct to handle ship type, if the index has a ship, if the index has been attacked
struct Coordinate{
  int shipType;
  bool hasShip;
  bool attacked;
};

//hit counter for win and lose conditions
int hitCounter;

//8x8 array to store board
Coordinate shipGrid[8][8];

//bool to handle ship orientation
bool ori = false;

//starting position for ships and cursor
int x = 3;
int y = 3;

void setup(){
  Serial.begin(9600);

  //for buttons
  pinMode(selectButton, INPUT);
  pinMode(orientButton, INPUT);

  //for leds
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);

  //for matrix
  matrix.shutdown(0,false);
  matrix.setIntensity(0,8);
  matrix.clearDisplay(0);

  //initialize number of ships to 3
  ships = 3;
  
  //setup intial board grid
  shipGridSetup();

  //YouAreReady retuns after placeships has finished running
  bool YouAreReady = placeShips();

  //light the green led once placeships function has finished running.
  digitalWrite(greenLed, HIGH);

  //Opponent ready bool to check if both players are ready
  bool OpponentReady = false; 

  //send the number 1 to the other arduino
  Serial.write(1);
  Serial.flush();

  //in a loop we check if the other arduino has sent us the number 1
  while(true){
    if(Serial.available()){
      int data = Serial.read();
      //once the number 1 is recieved then the opponent is ready
      if(data == 1){
        OpponentReady = true;
      }
    }
    //when both players are ready we break out of the loop and setup and get into the loop
    if(YouAreReady && OpponentReady){
      digitalWrite(greenLed, LOW);
      break;
    }
  }
}

void loop(){

  //check if myturn == true, if so we get into the attack phase.
  if(myTurn == true){
    attackPhase();
  }

  //if it isnt our turn then we have to wait and read the other arduinos input
  else{
    showShipGrid();

    //initalize serial communication
    if(Serial.available()){

      //read in data and check if the data sent from the other arduino is 2
      int data = Serial.read();
      if(data == 2){
        // x and y variables we need to read in from the other arduino
        int xRead = Serial.parseInt();
        int yRead = Serial.parseInt();

        //for debugging
        Serial.print("READ: ");
        Serial.print(xRead);
        Serial.print(", ");
        Serial.println(yRead);

        //Opponent Hit Condition
        //first we check if the x and y coordinate that was hit has a ship
        //if it does
        if(shipGrid[xRead][yRead].hasShip){
          removeShip(shipGrid[xRead][yRead].shipType);

          //for debugging
          Serial.println("HAS SHIP");

          //We send the number 9, it means that we have been hit
          //We send the bit to alert the other player that they have hit a ship
          Serial.write(9);

          //decrement our number of ships
          ships--;
        }

        //Opponent Miss Condition
        //if it does not
        else{
          //for debugging
          Serial.println("No Ship");

          //We send the number 8, it means that the other player has missed
          //We send the bit to alert the other player that they have missed a ship
          Serial.write(8);
        }
        //once the other player has made their move, their turn is over and it is now our turn.
        myTurn = true;
      }
    }
  }
  //loss condition
  //if we have 0 ships remaining then we reset the game
  if(ships == 0){
    delay(1200);

    //reset variables
    ships = 3;
    hitCounter = 0;

    //call setup to reset the game.
    setup();
  }
}

//Function for the gameWinner
void gameWinner(){

  //flash both leds to let the user know they have won
  for(int i = 0; i < 3; i++){
    digitalWrite(greenLed, HIGH);
    delay(200);
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, HIGH);
    delay(200);
    digitalWrite(redLed, LOW);
  }
  //send the number 5 to the other arduino
  Serial.write(5);

  //reset variables
  hitCounter = 0;
  ships = 3;

  //call setup to restart the game.
  setup();
}

//main attack phase function
//this fucntion will send data to the other arduino during the users attack phase.
void attackPhase(){

  //initalize confirm button
  int confirm = digitalRead(selectButton);

  delay(150);

  //call joystickCursor so user can move around the board
  joystickCursor();

  //clear the display
  matrix.clearDisplay(0);
  
  showAttackGrid();

  //set the cursor
  matrix.setLed(0, x, y, true);

  //if the user has pressed the confirm button then we check for some things
  if(confirm == HIGH){
    
    if(!shipGrid[x][y].attacked){
    shipGrid[x][y].attacked = true;

    //send the number 2, and the x and y coordinates to the other arduino
    Serial.write(2);
    Serial.write(x);
    Serial.write(y);

    //for debugging
    Serial.print("WRITE: ");
    Serial.print(x);
    Serial.print(", ");
    Serial.println(y);

    //loop to check if user has hit a ship or missed a ship.
    while(true){
      if(Serial.available()){
        int read = Serial.read();
        //HIT CONDITION
        if(read == 9){
          //for debugging
          Serial.println("HIT");

          //increment the hitCounter
          //we do this do that once the hitCounter is 3 we have a game winner.
          hitCounter++;

          //if the user hits all 3 of the other players ships then we call the gameWinner function
          if(hitCounter == 3){
            gameWinner();
          }

          //if we hit one of the other players ships then we flash the green led
          for(int i = 0; i < 3; i++){
            digitalWrite(greenLed, HIGH);
            delay(200);
            digitalWrite(greenLed, LOW);
            delay(200);
          }
          break;
        }
        //MISS CONDITION
        else if (read == 8){
          //for debugging
          Serial.println("MISS");

          //if we miss, then we flash the red led
          for(int i = 0; i < 3; i++){
            digitalWrite(redLed, HIGH);
            delay(200);
            digitalWrite(redLed, LOW);
            delay(200);
          }
          break;
        }
      }
    }

    //change myTurn to false at the end of your attack phase and clear the cursor.
    myTurn = false;
    matrix.clearDisplay(0); 
    }
  }

}

//function to remove the ship if it has been hit
//check the ship type first, and then change the hasShip flag for that x, y location to false.
void removeShip(int shipType){

  for(int x = 0; x < 8; x++){
    for(int y = 0; y < 8; y++){
      if(shipGrid[x][y].shipType == shipType){
        shipGrid[x][y].hasShip = false;
      }
    }
  }
}

//function to show the attackers grid
//set led cursor position
//leave it as a single ID
void showAttackGrid(){
  for(int x = 0; x < 8; x++){
    for(int y = 0; y < 8; y++){
      if(shipGrid[x][y].attacked == true){
        matrix.setLed(0, x, y, 1);
      }
    }
  }
}

//The placeships functions checks for the amount of ships the user has placed.
bool placeShips(){
  int shipsPlaced = 0;

  //the loop will run until the number of ships placed is 3
  while(shipsPlaced != 3){

    //first we place a ship of size 4
    if(shipsPlaced == 0){
      if(placeBigShip()){ shipsPlaced++; delay(200);}

    //Next, we place a ship of size 3
    }else if(shipsPlaced == 1){
      if(placeMediumShip()) {shipsPlaced++; delay(200);}

    //Finally, we place a ships of size 2
    }else if(shipsPlaced == 2){
      if(placeSmallShip()) {shipsPlaced++; delay(200);}
    }
  }
  return true;
}

//function to show grid.
//this function is primarily used to save the state once ships are placed
void showShipGrid(){
  for(int x = 0; x < 8; x++){
    for(int y = 0; y < 8; y++){
      if(shipGrid[x][y].hasShip == 1){
        matrix.setLed(0, x, y, 1);
      }
    }
  }
}

//check the coordinates for the ships
//this fucntion will be used for edge cases like when the user tries to place a ship over another one
//the else if accounts for when the ship is oriented.
//the ship that is trying to be placed on another ship will blink, the confirm button cannot be used.
bool checkCoordinates(int shipSize, bool orientation, int p1, int p2, int p3, int p4){
  //we check if the ship size if 3 because it is the second ship and the first ship has already been placed thus we cannot change its position
  if(shipSize == 3){
    if(orientation == 0){
      if(shipGrid[p1][p2].hasShip == true || shipGrid[p1][p3].hasShip == true || shipGrid[p1][p4].hasShip == true ){
        matrix.setLed(0, p1, p2, 0);
        matrix.setLed(0, p1, p3, 0);
        matrix.setLed(0, p1, p4, 0);
        delay(100);
        matrix.setLed(0, p1, p2, 1);
        matrix.setLed(0, p1, p3, 1);
        matrix.setLed(0, p1, p4, 1);
        return false;
      }
      return true;
    }else if(orientation == 1){
      if(shipGrid[p1][p4].hasShip == true || shipGrid[p2][p4].hasShip == true || shipGrid[p3][p4].hasShip == true){
        matrix.setLed(0, p1, p4, 0);
        matrix.setLed(0, p2, p4, 0);
        matrix.setLed(0, p3, p4, 0);
        delay(100);
        matrix.setLed(0, p1, p4, 1);
        matrix.setLed(0, p2, p4, 1);
        matrix.setLed(0, p3, p4, 1);
        return false;
      }
      return true;
    }
  //next we check if the ship size is 2
  //if it is then we blink the ship so that the user knows not to place a ship there.
  }else if(shipSize == 2){
    if(orientation == 0){
      if(shipGrid[p1][p2].hasShip == true || shipGrid[p1][p3].hasShip == true){
        matrix.setLed(0, p1, p2, 0);
        matrix.setLed(0, p1, p3, 0);
        delay(100);
        matrix.setLed(0, p1, p2, 1);
        matrix.setLed(0, p1, p3, 1);
        return false;
      }
      return true;
    }else if(orientation == 1){
      if(shipGrid[p1][p3].hasShip == true || shipGrid[p2][p3].hasShip == true){
        matrix.setLed(0, p1, p3, 0);
        matrix.setLed(0, p2, p3, 0);
        delay(100);
        matrix.setLed(0, p1, p3, 1);
        matrix.setLed(0, p2, p3, 1);
        return false;
      }
      return true;
    }
  }
}

//place the ships 
//this function places the ship on the board based on some factors
// these factors include the ships size, and the ships orientation
//it also changes the has ship flag to true once the ship is placed
//the has ship flag is used to make sure the user does not try to place a ship on another.
//we also update the shiptype based on its size
void place(int shipSize, bool orientation, int p1, int p2, int p3, int p4, int p5){
  //check if ship is size 4
  //if orientation is 0  then the ship is vetical, else it is horizontal
  if(shipSize == 4){
    if(orientation == 0){
      shipGrid[p1][p2].hasShip = true;
      shipGrid[p1][p2].shipType = 4;

      shipGrid[p1][p3].hasShip = true;
      shipGrid[p1][p3].shipType = 4;

      shipGrid[p1][p4].hasShip = true;
      shipGrid[p1][p4].shipType = 4;

      shipGrid[p1][p5].hasShip = true;
      shipGrid[p1][p5].shipType = 4;

    }else if(orientation == 1){
      shipGrid[p1][p5].hasShip = true;
      shipGrid[p1][p5].shipType = 4;

      shipGrid[p2][p5].hasShip = true;
      shipGrid[p2][p5].shipType = 4;

      shipGrid[p3][p5].hasShip = true;
      shipGrid[p3][p5].shipType = 4;

      shipGrid[p4][p5].hasShip = true;
      shipGrid[p4][p5].shipType = 4;
    }
    //check if ship is size 3
    //if orientation is 0  then the ship is vetical, else it is horizontal
  }else if(shipSize == 3){
    if(orientation == 0){
      shipGrid[p1][p2].hasShip = true;
      shipGrid[p1][p2].shipType = 3;

      shipGrid[p1][p3].hasShip = true;
      shipGrid[p1][p3].shipType = 3;

      shipGrid[p1][p4].hasShip = true;
      shipGrid[p1][p4].shipType = 3;

    }else if(orientation == 1){
      shipGrid[p1][p4].hasShip = true;
      shipGrid[p1][p4].shipType = 3;

      shipGrid[p2][p4].hasShip = true;
      shipGrid[p2][p4].shipType = 3;

      shipGrid[p3][p4].hasShip = true;
      shipGrid[p3][p4].shipType = 3;
    }
    //check is ship is size 2
    //if orientation is 0  then the ship is vetical, else it is horizontal
  }else if(shipSize == 2){
    if(orientation == 0){
      shipGrid[p1][p2].hasShip = true;
      shipGrid[p1][p2].shipType = 2;

      shipGrid[p1][p3].hasShip = true;
      shipGrid[p1][p3].shipType = 2;
    }else if(orientation == 1){
      shipGrid[p1][p3].hasShip = true;
      shipGrid[p1][p3].shipType = 2;

      shipGrid[p2][p3].hasShip = true;
      shipGrid[p2][p3].shipType = 2;
    }
  }
}

//Function to control the cursor movement for the joystick
void joystickCursor(){
  // Read joystick values
  int joyX = analogRead(A0);
  int joyY = analogRead(A1);

  //matrix.clearDisplay(0);

  // Update cursor position based on joystick input
  if (joyX > 900 && x < 7) {
    // Move cursor right
    matrix.setLed(0, x, y, false);
    x++;
    matrix.setLed(0, x, y, true);
  } else if (joyX < 100 && x > 0) {
    // Move cursor left
    matrix.setLed(0, x, y, false);
    x--;
    matrix.setLed(0, x, y, true);
  }
  if (joyY < 100 && y > 0) {
    // Move cursor up
    matrix.setLed(0, x, y, false);
    y--;
    matrix.setLed(0, x, y, true);
  } else if (joyY > 900 && y < 7) {
    // Move cursor down
    matrix.setLed(0, x, y, false);
    y++;
    matrix.setLed(0, x, y, true);

  }  
}

//Function to place a ship of size 4
//this also displays the ship
bool placeBigShip(){

  //for confirm button
  int confirm = digitalRead(selectButton);

  delay(150);

  //for orientation button
  if(digitalRead(orientButton) == HIGH){
    ori = !ori;
  }

  //call joystick cursor function so user can move their ship arpund
  joystickCursor();
  
  //clear the matrix
  matrix.clearDisplay(0);

  //show the initial ship grid
  showShipGrid();
  
  //ori is false when the ship is veritcal
  if(ori == false){
    if(y >= 4){
      //display ship
      matrix.setLed(0, x, 4, true);
      matrix.setLed(0, x, 5, true);
      matrix.setLed(0, x, 6, true);
      matrix.setLed(0, x, 7, true);
      if(confirm == HIGH){
        //if confirm is high then we place the ship using the place function
        place(4, 0, x, 4, 5, 6, 7);
        return true;
      }
    }else{
      matrix.setLed(0, x, y, true);
      matrix.setLed(0, x, y+1, true);
      matrix.setLed(0, x, y+2, true);
      matrix.setLed(0, x, y+3, true);
      if(confirm == HIGH){
        //if confirm is high then we place the ship using the place function
        place(4, 0, x, y, y+1, y+2, y+3);
        return true;
      }
    }
  //ori is true when the ship is horizontal
  //same behavior is applied to ship if it is horizontal
  }else{
    if(x >= 4){
      matrix.setLed(0, 4, y, true);
      matrix.setLed(0, 5, y, true);
      matrix.setLed(0, 6, y, true);
      matrix.setLed(0, 7, y, true);
      if(confirm == HIGH){
        //if confirm is high then we place the ship using the place function
        place(4, 1, 4, 5, 6, 7, y);
        return true;
      }
    }else{
      matrix.setLed(0, x, y, true);
      matrix.setLed(0, x+1, y, true);
      matrix.setLed(0, x+2, y, true);
      matrix.setLed(0, x+3, y, true);
      if(confirm == HIGH){
        //if confirm is high then we place the ship using the place function
        place(4, 1, x, x+1, x+2, x+3, y);
        return true;
      }
    }
  }
  return false;
}

//Function to place a ship of size 3
bool placeMediumShip(){
  int confirm = digitalRead(selectButton);

  delay(150);

  //change ori based on button input
  if(digitalRead(orientButton) == HIGH){
    ori = !ori;
  }

  //initialize joystick cursor
  joystickCursor();
  
  matrix.clearDisplay(0);
  showShipGrid();

  bool valid;

  //if ori is false then the ship is vertical
  if(ori == false){
    if(y >= 5){
      //check if ship is valid, i.e not trying to be placed on another ship
      valid = checkCoordinates(3, 0, x, 5, 6, 7);
      //display ship based on current cursor location
      matrix.setLed(0, x, 5, true);
      matrix.setLed(0, x, 6, true);
      matrix.setLed(0, x, 7, true);
      if(confirm == HIGH){
        if(valid){
          //if confirm is high then we place the ship using the place function
          place(3, 0, x, 5, 6, 7, -1);
          return true;
        }
      }
    }else{
      valid = checkCoordinates(3, 0, x, y, y+1, y+2);
      matrix.setLed(0, x, y, true);
      matrix.setLed(0, x, y+1, true);
      matrix.setLed(0, x, y+2, true);
      if(confirm == HIGH){
        if(valid){
          //if confirm is high then we place the ship using the place function
          place(3, 0, x, y, y+1, y+2, -1);
          return true;
        }
      }
    }
  //if ori is true then the ship is horizontal
  }else{
    if(x >= 5){
      valid = checkCoordinates(3, 1, 5, 6, 7, y);
      matrix.setLed(0, 5, y, true);
      matrix.setLed(0, 6, y, true);
      matrix.setLed(0, 7, y, true);
      if(confirm == HIGH){
        if(valid){
          //if confirm is high then we place the ship using the place function
          place(3, 1, 5, 6, 7, y, -1);
          return true;
        }
      }
    }else{
      valid = checkCoordinates(3, 1, x, x+1, x+2, y);
      matrix.setLed(0, x, y, true);
      matrix.setLed(0, x+1, y, true);
      matrix.setLed(0, x+2, y, true);
      if(confirm == HIGH){
        if(valid){
          //if confirm is high then we place the ship using the place function
          place(3, 1, x, x+1, x+2, y, -1);
          return true;
        }
      }
    }
  }
  return false;
}

//Function to place a ship of size 2
bool placeSmallShip(){
  int confirm = digitalRead(selectButton);

  delay(150);

  if(digitalRead(orientButton) == HIGH){
    ori = !ori;
  }

  joystickCursor();

  matrix.clearDisplay(0);
  showShipGrid();

  bool valid;

  //if ori is false then the ship is vertical
  if(ori == false){
    if(y >= 6){
      //check if ship is valid, i.e is not trying to be placed on another ship
      valid = checkCoordinates(2, 0, x, 6, 7, -1);
      //display ship based on current cursor location.
      matrix.setLed(0, x, 6, true);
      matrix.setLed(0, x, 7, true);
      if(confirm == HIGH){
        if(valid){
          //if confirm is high then we place the ship using the place function
          place(2, 0, x, 6, 7, -1, -1);
          return true;
        }
      }
    }else{
      valid = checkCoordinates(2, 0, x, y, y+1, -1);
      matrix.setLed(0, x, y, true);
      matrix.setLed(0, x, y+1, true);
      if(confirm == HIGH){
        if(valid){
          //if confirm is high then we place the ship using the place function
          place(2, 0, x, y, y+1, -1, -1);
          return true;
        }
      }
    }
  //if ori is true then the ship is horizontal
  //the same behavior is applied to the ship if it is horizontal.
  }else{
    if(x >= 6){
      valid = checkCoordinates(2, 1, 6, 7, y, -1);
      matrix.setLed(0, 6, y, true);
      matrix.setLed(0, 7, y, true);
      if(confirm == HIGH){
        if(valid){
          //if confirm is high then we place the ship using the place function
          place(2, 1, 6, 7, y, -1, -1);
          return true;
        }
      }
    }else{
      valid = checkCoordinates(2, 1, x, x+1, y, -1);
      matrix.setLed(0, x, y, true);
      matrix.setLed(0, x+1, y, true);
      if(confirm == HIGH){
        if(valid){
          //if confirm is high then we place the ship using the place function
          place(2, 1, x, x+1, y, -1, -1);
          return true;
        }
      }
    }
  }
  return false;
}

//Set up the inital ship grid
//we set has ship, attacked and shiptype to their starting values
//these values will be changed as the game goes on.
void shipGridSetup(){
  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
      shipGrid[i][j].hasShip = false;
      shipGrid[i][j].attacked = false;
      shipGrid[i][j].shipType = 0;
    }
  }
}