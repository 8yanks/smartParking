-- CreateTable
CREATE TABLE "Reservation" (
    "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    "spotId" INTEGER NOT NULL,
    "licensePlate" TEXT NOT NULL,
    "startTime" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "endTime" DATETIME NOT NULL,
    "durationMinutes" INTEGER NOT NULL,
    "amountPaid" REAL NOT NULL,
    "fineAmount" REAL NOT NULL DEFAULT 0,
    "status" TEXT NOT NULL DEFAULT 'ACTIVE',
    "createdAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "Reservation_spotId_fkey" FOREIGN KEY ("spotId") REFERENCES "ParkingSpot" ("id") ON DELETE RESTRICT ON UPDATE CASCADE
);

-- CreateIndex
CREATE INDEX "Reservation_spotId_status_idx" ON "Reservation"("spotId", "status");

-- CreateIndex
CREATE INDEX "Reservation_licensePlate_idx" ON "Reservation"("licensePlate");
