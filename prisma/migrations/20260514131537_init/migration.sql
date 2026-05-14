-- CreateTable
CREATE TABLE "ParkingSpot" (
    "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    "name" TEXT NOT NULL,
    "isOccupied" BOOLEAN NOT NULL DEFAULT false,
    "lastUpdatedAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "isActive" BOOLEAN NOT NULL DEFAULT true
);

-- CreateTable
CREATE TABLE "SensorData" (
    "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    "spotId" INTEGER NOT NULL,
    "isOccupied" BOOLEAN NOT NULL,
    "distanceCm" REAL NOT NULL,
    "esp32Id" TEXT NOT NULL,
    "recordedAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "SensorData_spotId_fkey" FOREIGN KEY ("spotId") REFERENCES "ParkingSpot" ("id") ON DELETE RESTRICT ON UPDATE CASCADE
);

-- CreateIndex
CREATE INDEX "SensorData_spotId_recordedAt_idx" ON "SensorData"("spotId", "recordedAt");
