-- CreateTable
CREATE TABLE "Barrier" (
    "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT DEFAULT 1,
    "isOpen" BOOLEAN NOT NULL DEFAULT false,
    "updatedAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- RedefineTables
PRAGMA defer_foreign_keys=ON;
PRAGMA foreign_keys=OFF;
CREATE TABLE "new_ParkingSpot" (
    "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    "name" TEXT NOT NULL,
    "isOccupied" BOOLEAN NOT NULL DEFAULT false,
    "isHandicap" BOOLEAN NOT NULL DEFAULT false,
    "lastUpdatedAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "isActive" BOOLEAN NOT NULL DEFAULT true
);
INSERT INTO "new_ParkingSpot" ("id", "isActive", "isOccupied", "lastUpdatedAt", "name") SELECT "id", "isActive", "isOccupied", "lastUpdatedAt", "name" FROM "ParkingSpot";
DROP TABLE "ParkingSpot";
ALTER TABLE "new_ParkingSpot" RENAME TO "ParkingSpot";
PRAGMA foreign_keys=ON;
PRAGMA defer_foreign_keys=OFF;
