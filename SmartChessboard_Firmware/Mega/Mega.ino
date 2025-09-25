#include "SmartKit.h"
#include <Arduino.h>
#include <EEPROM.h>

// === GLOBALS AND CONSTANTS ===
// These MUST be at the very top of the file, before any function uses them

float Z0 = -15;              // Board surface (no gripper)
float Z_gripper_zero = -15;  // Gripper tip on board

// Safety limits
#define MIN_Z_HEIGHT -150.0  // Modified to accommodate lower Z values
#define MAX_Z_HEIGHT 250.0
#define MIN_X_COORD 100.0
#define MAX_X_COORD 300.0
#define MIN_Y_COORD -100.0   // Modified to accommodate negative Y values
#define MAX_Y_COORD 300.0

// Maximum number of captured pieces per side
#define MAX_CAPTURED_PIECES 16

// Gripper XY offset relative to calibration tool (to center gripper on square)
float GRIPPER_OFFSET_X = 0.0; // Set after calibration (mm)
float GRIPPER_OFFSET_Y = 0.0; // Set after calibration (mm)
// To calibrate: place a pawn at a known square center, move gripper to calculated center, and measure X/Y offset needed to perfectly center gripper over pawn.

#define PIECE_HEIGHT_PAWN    32.097
#define PIECE_HEIGHT_ROOK    33.833
#define PIECE_HEIGHT_KNIGHT  43.674
#define PIECE_HEIGHT_BISHOP  47.894
#define PIECE_HEIGHT_QUEEN   49.984
#define PIECE_HEIGHT_KING    53.676

// Helper function to get pickup Z for a piece type (relative to gripper zero)

#include "Dobot.h"

// Set to 1 to try to use Dobot_GetPose (may cause linking errors)
// Set to 0 to use predefined coordinates
#define USE_DOBOT_GETPOSE 1

// (Corner calibration removed: now calibrate only edge square centers)

float matrix[8][8][3];  // Will be calculated based on corner positions

#if USE_DOBOT_GETPOSE
// Function to get coordinate by position type
float getCoordinate(int posType) {
    return Dobot_GetPose((Pos)posType); 
}
#endif

// EEPROM addresses for calibration data
#define EEPROM_CALIBRATION_VALID 0    // 1 byte
#define EEPROM_Z0 1                   // 4 bytes
#define EEPROM_Z_GRIPPER_ZERO 5       // 4 bytes
#define EEPROM_MATRIX_START 9         // 8*8*3*4 = 768 bytes

// Higher pickup height for safety
#define SAFE_PICKUP_HEIGHT 35  // Increased height for piece pickup

// Altezze di movimento sicure
#define SAFE_TRAVEL_HEIGHT 37.0    // Altezza di viaggio normale
#define SAFE_TRAVEL_HEIGHT_EDGE 27.0 // Altezza di viaggio ridotta per colonne a e h
#define PIECE_GRAB_OFFSET 5.0      // Offset per la presa dei pezzi

// Velocità di movimento
#define FAST_SPEED 100            // Aumentata da 50 a 100
#define SLOW_SPEED 50             // Aumentata da 20 a 50

// Area di deposito per i pezzi catturati
#define CAPTURED_PIECES_X 200     // Posizione X dell'area pezzi catturati
#define CAPTURED_PIECES_Y 200     // Posizione Y dell'area pezzi catturati
#define CAPTURED_PIECES_Z 0       // Altezza base dell'area pezzi catturati
#define PIECES_SPACING 30         // Spazio tra i pezzi catturati

// Contatori per i pezzi catturati
int capturedWhitePieces = 0;
int capturedBlackPieces = 0;

// Constants for the chess robot
#define CAPTURED_PIECE_AREA_X 200.0  // X coordinate for captured pieces
#define CAPTURED_PIECE_AREA_Y 200.0  // Y coordinate for captured pieces
#define CAPTURED_PIECE_SPACING 30.0   // Spacing between captured pieces

// Game state
bool gameInProgress = false;
int capturedPieceCount = 0;

// Calibration variables
bool isCalibrated = false;
bool isEmergencyStop = false;

// EEPROM addresses
#define EEPROM_CALIBRATED_FLAG 0
#define EEPROM_Z0_ADDR 1
#define EEPROM_MATRIX_ADDR 5

// Function declarations
bool validateCoordinates(float x, float y, float z);
void emergencyStop();
bool initializeLEDs();
void handleSerialInput();
void processMKRCommand(String data);
void printStartupInfo();
void printHelp();
void printSystemStatus();
void printCalibrationStatus();
void testDobotMovement();
void testGripper();
void simulateMove(String move);

// Function to save calibration data to EEPROM
void saveCalibrationToEEPROM() {
    // Write validation byte
    EEPROM.write(EEPROM_CALIBRATION_VALID, 0xAA);
    
    // Write Z values
    EEPROM.put(EEPROM_Z0, Z0);
    EEPROM.put(EEPROM_Z_GRIPPER_ZERO, Z_gripper_zero);
    
    // Write matrix data
    int addr = EEPROM_MATRIX_START;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            for (int coord = 0; coord < 3; coord++) {
                EEPROM.put(addr, matrix[row][col][coord]);
                addr += sizeof(float);
            }
        }
    }
}

// Function to load calibration data from EEPROM
bool loadCalibrationFromEEPROM() {
    // Check if valid calibration exists
    if (EEPROM.read(EEPROM_CALIBRATION_VALID) != 0xAA) {
        return false;
    }
    
    // Read Z values
    EEPROM.get(EEPROM_Z0, Z0);
    EEPROM.get(EEPROM_Z_GRIPPER_ZERO, Z_gripper_zero);
    
    // Read matrix data
    int addr = EEPROM_MATRIX_START;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            for (int coord = 0; coord < 3; coord++) {
                EEPROM.get(addr, matrix[row][col][coord]);
                addr += sizeof(float);
            }
        }
    }
    return true;
}

void setup() {
    Serial.begin(115200);  // Initialize serial communication for debugging
    Serial1.begin(9600);   // Initialize Serial1 for communication with ESP32
    
    // LED system removed - now handled by MKR
    
    // Initialize Dobot and home position
    Dobot_Init();
    
    // Try to load calibration from EEPROM
    if (loadCalibrationFromEEPROM()) {
        isCalibrated = true;
        Serial.println("Calibration loaded from EEPROM");
        Serial1.println("CALIB_MSG:Calibration loaded from EEPROM");
    }
    
    // Print startup information
    printStartupInfo();
    printHelp();
}

// LED functions removed - now handled by MKR

bool validateCoordinates(float x, float y, float z) {
    if (z < MIN_Z_HEIGHT || z > MAX_Z_HEIGHT) {
        Serial.println("ERROR: Z coordinate out of safe range!");
        return false;
    }
    if (x < MIN_X_COORD || x > MAX_X_COORD) {
        Serial.println("ERROR: X coordinate out of safe range!");
        return false;
    }
    if (y < MIN_Y_COORD || y > MAX_Y_COORD) {
        Serial.println("ERROR: Y coordinate out of safe range!");
        return false;
    }
    return true;
}

void emergencyStop() {
    isEmergencyStop = true;
    Serial.println("EMERGENCY STOP ACTIVATED!");
    Serial1.println("CALIB_MSG:EMERGENCY STOP ACTIVATED!");
    
    // Open gripper to release any held piece
    Dobot_SetEndEffectorGripper(true, false);
    delay(1000);
    Dobot_SetEndEffectorGripper(false, false);
    
    // LED control removed - now handled by MKR
}

void calibrateChessboard() {
    bool confirmed = false;
    Serial.println("\n=== STARTING CALIBRATION ===");
    Serial1.println("CALIB_MSG:Starting chessboard calibration...");
    
    // Home position
    Serial.println("Moving to home position...");
    Dobot_SetHOMECmd();
    delay(2000);

    // LED control removed - now handled by MKR

    // 1. Calibrate Z0 (board surface without gripper)
    Serial.println("\nSTEP 1: Calibrating board surface height (Z0)");
    Serial1.println("CALIB_MSG:STEP 1: Place the calibration tool on the board surface. Press confirm when ready.");
    
    confirmed = false;
    while (!confirmed) {
        if (Serial1.available()) {
            String conf = Serial1.readStringUntil('\n');
            conf.trim();
            if (conf.equalsIgnoreCase("CALIB_CONFIRM")) { confirmed = true; break; }
        }
        delay(10);
    }
    Z0 = getCoordinate(Z);
    Serial.print("Z0 (board surface) saved as: "); Serial.println(Z0);
    Serial1.print("CALIB_MSG:Z0 saved as: "); Serial1.println(Z0);

    // 2. Calibrate Z_gripper_zero (with gripper)
    Serial.println("\nSTEP 2: Calibrating gripper height (Z_gripper_zero)");
    Serial1.println("CALIB_MSG:STEP 2: Attach gripper and place it on board surface. Press confirm when ready.");
    
    confirmed = false;
    while (!confirmed) {
        if (Serial1.available()) {
            String conf = Serial1.readStringUntil('\n');
            conf.trim();
            if (conf.equalsIgnoreCase("CALIB_CONFIRM")) { confirmed = true; break; }
        }
        delay(10);
    }
    Z_gripper_zero = getCoordinate(Z);
    Serial.print("Z_gripper_zero saved as: "); Serial.println(Z_gripper_zero);
    Serial1.print("CALIB_MSG:Z_gripper_zero saved as: "); Serial1.println(Z_gripper_zero);

    // LED control removed - now handled by MKR

    // 3. Calibrate corner positions
    Serial.println("\nSTEP 3: Calibrating corner positions");
    float corners[4][2];  // Store X,Y coordinates of corners
    String cornerNames[4] = {"a8", "h8", "a1", "h1"};
    int cornerRows[4] = {0, 0, 7, 7};
    int cornerCols[4] = {0, 7, 0, 7};
    
    for (int i = 0; i < 4; i++) {
        Serial.print("\nCalibrating corner "); Serial.println(cornerNames[i]);
        Serial1.println("CALIB_MSG:Move to " + cornerNames[i] + " corner and press confirm when ready");
        
        // LED control removed - now handled by MKR
        
    confirmed = false;
    while (!confirmed) {
        if (Serial1.available()) {
            String conf = Serial1.readStringUntil('\n');
            conf.trim();
            if (conf.equalsIgnoreCase("CALIB_CONFIRM")) { confirmed = true; break; }
        }
        delay(10);
    }
        corners[i][0] = getCoordinate(X);
        corners[i][1] = getCoordinate(Y);
        Serial.print(cornerNames[i] + " position: X=");
        Serial.print(corners[i][0]); Serial.print(" Y=");
        Serial.println(corners[i][1]);
        Serial1.print("CALIB_MSG:" + cornerNames[i] + " saved as X=");
        Serial1.print(corners[i][0]); Serial1.print(" Y=");
        Serial1.println(corners[i][1]);
    }

    // Calculate all square positions
    Serial.println("\nCalculating square positions...");
    float xStep = (corners[1][0] - corners[0][0]) / 7.0;
    float yStep = (corners[2][1] - corners[0][1]) / 7.0;
    
    // LED control removed - now handled by MKR
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            matrix[row][col][0] = corners[0][0] + col * xStep;  // X coordinate
            matrix[row][col][1] = corners[0][1] + row * yStep;  // Y coordinate
            matrix[row][col][2] = Z_gripper_zero;  // Z coordinate
        }
    }
    
    // Save calibration to EEPROM
    Serial.println("\nSaving calibration to EEPROM...");
    saveCalibrationToEEPROM();
    
    // Verify EEPROM save
    Serial.println("Verifying EEPROM data...");
    float testZ0, testZ_gripper;
    EEPROM.get(EEPROM_Z0, testZ0);
    EEPROM.get(EEPROM_Z_GRIPPER_ZERO, testZ_gripper);
    
    if (testZ0 != Z0 || testZ_gripper != Z_gripper_zero) {
        Serial.println("ERROR: EEPROM verification failed!");
        Serial1.println("CALIB_MSG:ERROR: EEPROM verification failed!");
        isCalibrated = false;
        return;
    }
    
    // Final verification
    Serial.println("\nCalibration values:");
    Serial.print("Z0 (board surface): "); Serial.println(Z0);
    Serial.print("Z_gripper_zero: "); Serial.println(Z_gripper_zero);
    Serial.print("Gripper offset: "); Serial.println(Z_gripper_zero - Z0);
    
    delay(1000);
    
    isCalibrated = true;
    Serial.println("\n=== CALIBRATION COMPLETE ===");
    Serial1.println("CALIB_MSG:Calibration complete and verified!");
}

// Calculate the matrix positions based on the edge positions
// Helper function to get pickup Z for a piece type (relative to gripper zero, no offset)

// Helper function to get pickup Z for a piece type (relative to gripper zero, no offset)

// Calculate the matrix positions based on the corner positions
void calculateMatrixPositions() {
    // Bilinear interpolation from the four corners
    float x_a1 = matrix[7][0][0]; float y_a1 = matrix[7][0][1];
    float x_h1 = matrix[7][7][0]; float y_h1 = matrix[7][7][1];
    float x_a8 = matrix[0][0][0]; float y_a8 = matrix[0][0][1];
    float x_h8 = matrix[0][7][0]; float y_h8 = matrix[0][7][1];

    // Validate corner positions
    if (!validateCoordinates(x_a1, y_a1, Z0) ||
        !validateCoordinates(x_h1, y_h1, Z0) ||
        !validateCoordinates(x_a8, y_a8, Z0) ||
        !validateCoordinates(x_h8, y_h8, Z0)) {
        Serial.println("ERROR: Invalid corner coordinates!");
        Serial1.println("CALIB_MSG:ERROR: Invalid corner coordinates!");
        isCalibrated = false;
        return;
    }

    for (int row = 0; row < 8; row++) {
        float tRow = row / 7.0;
        for (int col = 0; col < 8; col++) {
            float tCol = col / 7.0;

            // Corrected bilinear interpolation
            // X coordinate
            matrix[row][col][0] =
                (1 - tRow) * (1 - tCol) * x_a1 +
                (1 - tRow) * tCol       * x_h1 +
                tRow       * (1 - tCol) * x_a8 +
                tRow       * tCol       * x_h8;

            // Y coordinate (corrected order)
            matrix[row][col][1] =
                (1 - tRow) * (1 - tCol) * y_a1 +
                (1 - tRow) * tCol       * y_h1 +
                tRow       * (1 - tCol) * y_a8 +
                tRow       * tCol       * y_h8;

            // Z coordinate with validation
            matrix[row][col][2] = Z_gripper_zero;
            
            // Validate calculated position
            if (!validateCoordinates(matrix[row][col][0], 
                                   matrix[row][col][1], 
                                   matrix[row][col][2])) {
                Serial.println("ERROR: Invalid calculated position!");
                Serial1.println("CALIB_MSG:ERROR: Invalid calculated position!");
                isCalibrated = false;
                return;
            }
        }
    }

    // Print calculated positions for verification
    Serial.println("\n--- CALCULATED MATRIX ---");
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Serial.print((char)('a' + col));
            Serial.print(8 - row);
            Serial.print(": X=");
            Serial.print(matrix[row][col][0], 2);
            Serial.print(" Y=");
            Serial.print(matrix[row][col][1], 2);
            Serial.print(" Z=");
            Serial.println(matrix[row][col][2], 2);
        }
    }
    Serial.println("-------------------------");
    
    isCalibrated = true;
}

int charToIndex(char c) {
    return c - 'a';
}

void loop() {
    // Check for emergency stop condition
    if (isEmergencyStop) {
        return;
    }

    // Handle input from monitor seriale (Serial)
    handleSerialInput();
    
    // Check if data is available on Serial1 (from MKR)
    if (Serial1.available()) {
        String data = Serial1.readStringUntil('\n');
        data.trim();
        Serial.println("Received from MKR: " + data);
        
        // Process commands from MKR
        processMKRCommand(data);
    }
}

// Function to process move data
void processMoveData(const String& moveData) {
    if (isEmergencyStop) {
        Serial.println("ERROR: Emergency stop is active!");
        return;
    }

    String move = moveData;
    move.trim();
    Serial.println("\n=== PROCESSING MOVE ===");
    Serial.print("Raw move data: '");
    Serial.print(move);
    Serial.println("'");

    int fromCol, fromRow, toCol, toRow;
    bool isCapture = false;

    // Parse and validate move format
    if (!parseMoveData(move, fromCol, fromRow, toCol, toRow, isCapture)) {
        Serial.println("ERROR: Invalid move format!");
        return;
    }

    Serial.println("Move parsed successfully:");
    Serial.print("From: row="); Serial.print(fromRow); Serial.print(" col="); Serial.println(fromCol);
    Serial.print("To: row="); Serial.print(toRow); Serial.print(" col="); Serial.println(toCol);
    Serial.print("Is capture: "); Serial.println(isCapture ? "yes" : "no");

    // Get coordinates
    float fromX = matrix[fromRow][fromCol][0];
    float fromY = matrix[fromRow][fromCol][1];
    float fromZ = matrix[fromRow][fromCol][2];
    float toX = matrix[toRow][toCol][0];
    float toY = matrix[toRow][toCol][1];
    float toZ = matrix[toRow][toCol][2];

    Serial.println("Coordinates calculated:");
    Serial.print("From: X="); Serial.print(fromX); Serial.print(" Y="); Serial.print(fromY); Serial.print(" Z="); Serial.println(fromZ);
    Serial.print("To: X="); Serial.print(toX); Serial.print(" Y="); Serial.print(toY); Serial.print(" Z="); Serial.println(toZ);

    // Validate all coordinates
    if (!validateCoordinates(fromX, fromY, fromZ) ||
        !validateCoordinates(toX, toY, toZ)) {
        Serial.println("ERROR: Invalid move coordinates!");
        return;
    }

    // LED control removed - now handled by MKR

    // If it's a capture, first remove the captured piece
    if (isCapture) {
        if (!handleCapture(toX, toY, toZ)) {
            Serial.println("ERROR: Failed to handle capture!");
            return;
        }
    }

    // Check if calibration is valid before executing move
    if (!isCalibrated) {
        Serial.println("ERROR: System not calibrated!");
        return;
    }

    // Execute the move
    Serial.println("Executing move...");
    if (!executeMove(fromX, fromY, fromZ, toX, toY, toZ)) {
        Serial.println("ERROR: Move execution failed!");
        return;
    }

    Serial.println("=== MOVE COMPLETE ===\n");
    // LED control removed - now handled by MKR
}

bool parseMoveData(const String& move, int& fromCol, int& fromRow, int& toCol, int& toRow, bool& isCapture) {
    // Check if it's a capture move (format: e4xd5)
    if (move.indexOf('x') != -1) {
        if (move.length() != 5) return false;
        isCapture = true;
        fromCol = move.charAt(0) - 'a';
        fromRow = 8 - (move.charAt(1) - '0');
        toCol = move.charAt(3) - 'a';
        toRow = 8 - (move.charAt(4) - '0');
    } else if (move.length() == 4) {
        isCapture = false;
        fromCol = move.charAt(0) - 'a';
        fromRow = 8 - (move.charAt(1) - '0');
        toCol = move.charAt(2) - 'a';
        toRow = 8 - (move.charAt(3) - '0');
    } else {
        return false;
    }

    // Validate indices
    if (fromCol < 0 || fromCol > 7 || toCol < 0 || toCol > 7 ||
        fromRow < 0 || fromRow > 7 || toRow < 0 || toRow > 7) {
        return false;
    }

    return true;
}

bool handleCapture(float toX, float toY, float toZ) {
    Serial.println("\n=== CAPTURING PIECE ===");
    
    // Check if we have space for more captured pieces
    bool isWhitePiece = toY < (MIN_Y_COORD + MAX_Y_COORD) / 2;
    if ((isWhitePiece && capturedWhitePieces >= MAX_CAPTURED_PIECES) ||
        (!isWhitePiece && capturedBlackPieces >= MAX_CAPTURED_PIECES)) {
        Serial.println("ERROR: No more space for captured pieces!");
        return false;
    }

    float depositX, depositY;
    
    // Calculate deposit position
    if (!isWhitePiece) {
        depositX = CAPTURED_PIECES_X;
        depositY = CAPTURED_PIECES_Y + (PIECES_SPACING * capturedBlackPieces);
    } else {
        depositX = CAPTURED_PIECES_X;
        depositY = CAPTURED_PIECES_Y - (PIECES_SPACING * capturedWhitePieces);
    }

    // Validate deposit coordinates
    if (!validateCoordinates(depositX, depositY, CAPTURED_PIECES_Z)) {
        Serial.println("ERROR: Invalid deposit coordinates!");
        return false;
    }

    // Execute capture movement sequence
    if (!executeMove(toX, toY, toZ, depositX, depositY, CAPTURED_PIECES_Z)) {
        return false;
    }

    // Update counters only after successful capture
    if (isWhitePiece) {
        capturedWhitePieces++;
    } else {
        capturedBlackPieces++;
    }

    return true;
}

// Funzione per determinare l'altezza di viaggio sicura in base alla colonna
float getSafeTravelHeight(int col) {
    // Se la colonna è 'a' (0) o 'h' (7), usa l'altezza ridotta
    if (col == 0 || col == 7) {
        return SAFE_TRAVEL_HEIGHT_EDGE;
    }
    return SAFE_TRAVEL_HEIGHT;
}

bool executeMove(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) {
    Serial.println("\n=== EXECUTING MOVE ===");
    
    // Determina le colonne di partenza e arrivo (0-7, dove 0='a' e 7='h')
    int fromCol = round((fromX - matrix[0][0][0]) / ((matrix[0][7][0] - matrix[0][0][0]) / 7.0));
    int toCol = round((toX - matrix[0][0][0]) / ((matrix[0][7][0] - matrix[0][0][0]) / 7.0));
    
    // Determina l'altezza di viaggio in base alle colonne
    float travelHeight = min(getSafeTravelHeight(fromCol), getSafeTravelHeight(toCol));
    
    Serial.print("Move from column: ");
    Serial.print((char)('a' + fromCol));
    Serial.print(" to column: ");
    Serial.print((char)('a' + toCol));
    Serial.print(" - Using travel height: ");
    Serial.println(travelHeight);
    
    // Array of movement steps with dynamic height
    struct {
        const char* description;
        float x, y, z;
        float speed;
    } steps[] = {
        {"Moving to safe height above source", fromX, fromY, travelHeight - 15.0, FAST_SPEED},
        {"Moving down to pickup position", fromX, fromY, fromZ + PIECE_GRAB_OFFSET, SLOW_SPEED},
        {"Moving to safe height with piece", fromX, fromY, travelHeight - 15.0, FAST_SPEED},
        {"Moving above destination", toX, toY, travelHeight - 15.0, FAST_SPEED},
        {"Moving down to place position", toX, toY, toZ + PIECE_GRAB_OFFSET, SLOW_SPEED},
        {"Moving to final safe height", toX, toY, travelHeight - 15.0, FAST_SPEED}
    };

    // Execute each movement step
    for (int i = 0; i < 6; i++) {
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.print(steps[i].description);
        Serial.print(" (Z=");
        Serial.print(steps[i].z);
        Serial.println(")");

        if (!validateCoordinates(steps[i].x, steps[i].y, steps[i].z)) {
            Serial.println("ERROR: Invalid coordinates in movement sequence!");
            return false;
        }

        // Execute movement with speed control
        Dobot_SetPTPCmd(MOVJ_XYZ, steps[i].x, steps[i].y, steps[i].z, steps[i].speed);
        
        // Minimal delay for movement completion
        if (i == 1 || i == 4) { // Solo per pickup e place
            delay(500);
        }

        // Handle gripper operations
        if (i == 1) {
            Dobot_SetEndEffectorGripper(true, true);  // Close gripper
            delay(300);
        } else if (i == 4) {
            Dobot_SetEndEffectorGripper(true, false);  // Open gripper
            delay(300);
            Dobot_SetEndEffectorGripper(false, false); // Deactivate gripper
        }
    }

    return true;
}

// LED functions removed - now handled by MKR

// ===== FUNZIONI PER INPUT DA MONITOR SERIALE =====

void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.length() > 0) {
            Serial.println("Comando ricevuto: " + input);
            
            // Gestisce comandi di test
            if (input == "help" || input == "?") {
                printHelp();
            }
            else if (input == "status") {
                printSystemStatus();
            }
            else if (input == "calib") {
                printCalibrationStatus();
            }
            else if (input == "calibrate") {
                calibrateChessboard();
            }
            else if (input == "start") {
                if (!isCalibrated) {
                    Serial.println("ERROR: Cannot start game without calibration!");
                    return;
                }
                gameInProgress = true;
                capturedPieceCount = 0;
                capturedWhitePieces = 0;
                capturedBlackPieces = 0;
                Serial.println("Game started");
            }
            else if (input == "stop") {
                gameInProgress = false;
                Serial.println("Game stopped");
            }
            else if (input == "test") {
                testDobotMovement();
            }
            else if (input == "gripper") {
                testGripper();
            }
            else if (input.startsWith("move ")) {
                String move = input.substring(5);
                simulateMove(move);
            }
            else if (input == "emergency") {
                emergencyStop();
            }
            else if (input == "reset") {
                isEmergencyStop = false;
                Serial.println("Emergency stop reset");
            }
            else if (input == "home") {
                Serial.println("Moving to home position...");
                Dobot_SetPTPCmd(MOVJ_XYZ, 200, 0, 50, 50);
            }
            else {
                Serial.println("Comando non riconosciuto. Digita 'help' per vedere i comandi disponibili.");
            }
        }
    }
}

void processMKRCommand(String data) {
    // Process commands from MKR
    if (data.equalsIgnoreCase("CALIBRATE")) {
        calibrateChessboard();
    } else if (data.equalsIgnoreCase("STARTGAME")) {
        if (!isCalibrated) {
            Serial.println("ERROR: Cannot start game without calibration!");
            Serial1.println("CALIB_MSG:ERROR: Cannot start game without calibration!");
            return;
        }
        gameInProgress = true;
        capturedPieceCount = 0;
        capturedWhitePieces = 0;
        capturedBlackPieces = 0;
        Serial.println("Game started");
    } else if (data.equalsIgnoreCase("ENDGAME")) {
        gameInProgress = false;
        Serial.println("Game ended");
    } else if (data.startsWith("HIGHLIGHT:")) {
        String squares = data.substring(9);
        Serial.print("DEBUG - Highlighting squares string: '");
        Serial.print(squares);
        Serial.println("'");
    } else if (data.equalsIgnoreCase("CLEAR")) {
        // LED control removed - now handled by MKR
    } else if (gameInProgress) {
        if (!isCalibrated) {
            Serial.println("ERROR: Cannot make moves without calibration!");
            return;
        }
        processMoveData(data);
    } else {
        Serial.println("Game not in progress. Start game first.");
    }
}

void printStartupInfo() {
    Serial.println("========================================");
    Serial.println("SmartChessboard MEGA - Dobot Controller");
    Serial.println("========================================");
    Serial.print("Firmware Version: 1.0.0");
    Serial.print(" | Build Date: ");
    Serial.println(__DATE__ " " __TIME__);
    Serial.print("Calibration Status: ");
    Serial.println(isCalibrated ? "CALIBRATED" : "NOT CALIBRATED");
    Serial.print("Game Status: ");
    Serial.println(gameInProgress ? "IN PROGRESS" : "STOPPED");
    Serial.print("Emergency Stop: ");
    Serial.println(isEmergencyStop ? "ACTIVE" : "INACTIVE");
    Serial.println("========================================");
}

void printHelp() {
    Serial.println("========================================");
    Serial.println("COMANDI DISPONIBILI:");
    Serial.println("========================================");
    Serial.println("help, ?          - Mostra questo aiuto");
    Serial.println("status           - Stato del sistema");
    Serial.println("calib            - Stato calibrazione");
    Serial.println("calibrate        - Avvia calibrazione");
    Serial.println("start            - Avvia partita");
    Serial.println("stop             - Ferma partita");
    Serial.println("test             - Test movimento Dobot");
    Serial.println("gripper          - Test gripper");
    Serial.println("move e2e4        - Simula mossa (es: e2e4)");
    Serial.println("emergency        - Stop di emergenza");
    Serial.println("reset            - Reset stop di emergenza");
    Serial.println("home             - Vai a posizione home");
    Serial.println("========================================");
}

void printSystemStatus() {
    Serial.println("========================================");
    Serial.println("STATO SISTEMA:");
    Serial.println("========================================");
    Serial.print("Calibrazione: ");
    Serial.println(isCalibrated ? "Completata" : "Non completata");
    Serial.print("Partita: ");
    Serial.println(gameInProgress ? "In corso" : "Fermata");
    Serial.print("Stop di emergenza: ");
    Serial.println(isEmergencyStop ? "Attivo" : "Inattivo");
    Serial.print("Pezzi catturati bianchi: ");
    Serial.println(capturedWhitePieces);
    Serial.print("Pezzi catturati neri: ");
    Serial.println(capturedBlackPieces);
    Serial.print("Z0 (superficie scacchiera): ");
    Serial.println(Z0);
    Serial.print("Z_gripper_zero: ");
    Serial.println(Z_gripper_zero);
    Serial.println("========================================");
}

void printCalibrationStatus() {
    Serial.println("========================================");
    Serial.println("STATO CALIBRAZIONE:");
    Serial.println("========================================");
    Serial.print("Calibrazione valida: ");
    Serial.println(isCalibrated ? "Sì" : "No");
    if (isCalibrated) {
        Serial.print("Z0 (superficie): ");
        Serial.println(Z0);
        Serial.print("Z_gripper_zero: ");
        Serial.println(Z_gripper_zero);
        Serial.print("Offset gripper X: ");
        Serial.println(GRIPPER_OFFSET_X);
        Serial.print("Offset gripper Y: ");
        Serial.println(GRIPPER_OFFSET_Y);
        Serial.println("Matrice 8x8 caricata da EEPROM");
    } else {
        Serial.println("Esegui 'calibrate' per calibrare il sistema");
    }
    Serial.println("========================================");
}

void testDobotMovement() {
    Serial.println("Test movimento Dobot...");
    
    if (!isCalibrated) {
        Serial.println("ERROR: Sistema non calibrato!");
        return;
    }
    
    // Test movimento sicuro
    Serial.println("1. Movimento a posizione sicura...");
    Dobot_SetPTPCmd(MOVJ_XYZ, 200, 0, 50, 50);
    delay(2000);
    
    Serial.println("2. Test movimento a coordinate scacchiera...");
    // Usa coordinate della matrice se disponibile
    if (matrix[0][0][0] != 0) {
        float testX = matrix[0][0][0];
        float testY = matrix[0][0][1];
        float testZ = matrix[0][0][2] + 20; // 20mm sopra la scacchiera
        
        Serial.print("Movimento a: X=");
        Serial.print(testX);
        Serial.print(", Y=");
        Serial.print(testY);
        Serial.print(", Z=");
        Serial.println(testZ);
        
        Dobot_SetPTPCmd(MOVJ_XYZ, testX, testY, testZ, 50);
        delay(3000);
    }
    
    Serial.println("3. Ritorno a posizione home...");
    Dobot_SetPTPCmd(MOVJ_XYZ, 200, 0, 50, 50);
    delay(2000);
    
    Serial.println("Test movimento completato!");
}

void testGripper() {
    Serial.println("Test gripper...");
    
    Serial.println("1. Apertura gripper...");
    Dobot_SetEndEffectorGripper(true, false);
    delay(1000);
    
    Serial.println("2. Chiusura gripper...");
    Dobot_SetEndEffectorGripper(true, true);
    delay(1000);
    
    Serial.println("3. Apertura gripper...");
    Dobot_SetEndEffectorGripper(true, false);
    delay(1000);
    
    Serial.println("4. Disattivazione gripper...");
    Dobot_SetEndEffectorGripper(false, false);
    delay(1000);
    
    Serial.println("Test gripper completato!");
}

void simulateMove(String move) {
    Serial.println("Simulando mossa: " + move);
    
    if (!isCalibrated) {
        Serial.println("ERROR: Sistema non calibrato!");
        return;
    }
    
    if (!gameInProgress) {
        Serial.println("ERROR: Partita non in corso!");
        return;
    }
    
    // Processa la mossa come se fosse arrivata dal MKR
    processMoveData(move);
}
