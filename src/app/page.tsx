"use client";

import { useState, useEffect, useCallback } from "react";

type Reservation = {
  id: number;
  spotId: number;
  licensePlate: string;
  startTime: string;
  endTime: string;
  durationMinutes: number;
  amountPaid: number;
  fineAmount: number;
  status: string;
};

type Spot = {
  id: number;
  name: string;
  is_occupied: boolean;
  is_active: boolean;
  activeReservation: Reservation | null;
};

type SpotStatus = "LIBRE" | "RESERVE" | "EN_COURS" | "OCCUPE" | "DEPASSE";

const DURATIONS = [
  { label: "30 min", minutes: 30, price: 1.0 },
  { label: "1 h",   minutes: 60, price: 2.0 },
  { label: "2 h",   minutes: 120, price: 4.0 },
  { label: "3 h",   minutes: 180, price: 6.0 },
];

const STATUS_CFG: Record<SpotStatus, { bg: string; border: string; label: string; labelColor: string; dot: string }> = {
  LIBRE:    { bg: "bg-green-900/30",  border: "border-green-600",  label: "LIBRE",    labelColor: "text-green-400",  dot: "bg-green-500" },
  RESERVE:  { bg: "bg-blue-900/30",   border: "border-blue-600",   label: "RÉSERVÉ",  labelColor: "text-blue-400",   dot: "bg-blue-500" },
  EN_COURS: { bg: "bg-blue-900/50",   border: "border-blue-400",   label: "EN COURS", labelColor: "text-blue-300",   dot: "bg-blue-400" },
  OCCUPE:   { bg: "bg-red-900/30",    border: "border-red-600",    label: "OCCUPÉ",   labelColor: "text-red-400",    dot: "bg-red-500" },
  DEPASSE:  { bg: "bg-orange-900/30", border: "border-orange-500", label: "DÉPASSÉ",  labelColor: "text-orange-400", dot: "bg-orange-500" },
};

function getStatus(spot: Spot, now: Date): SpotStatus {
  const res = spot.activeReservation;
  if (spot.is_occupied) {
    if (!res) return "OCCUPE";
    if (res.status === "EXPIRED" || new Date(res.endTime) < now) return "DEPASSE";
    return "EN_COURS";
  }
  if (res && res.status === "ACTIVE" && new Date(res.endTime) > now) return "RESERVE";
  return "LIBRE";
}

function countdown(endTime: string, now: Date): string {
  const ms = new Date(endTime).getTime() - now.getTime();
  if (ms <= 0) return "00:00";
  const totalSec = Math.floor(ms / 1000);
  const h = Math.floor(totalSec / 3600);
  const m = Math.floor((totalSec % 3600) / 60);
  const s = totalSec % 60;
  if (h > 0) return `${h}h${String(m).padStart(2, "0")}`;
  return `${String(m).padStart(2, "0")}:${String(s).padStart(2, "0")}`;
}

export default function Home() {
  const [plate, setPlate] = useState("");
  const [inputPlate, setInputPlate] = useState("");
  const [spots, setSpots] = useState<Spot[]>([]);
  const [selectedSpot, setSelectedSpot] = useState<Spot | null>(null);
  const [duration, setDuration] = useState(DURATIONS[1]);
  const [step, setStep] = useState<"plate" | "grid" | "reserve" | "success">("plate");
  const [now, setNow] = useState(() => new Date());
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [confirmed, setConfirmed] = useState<(Reservation & { spotName: string }) | null>(null);

  const fetchSpots = useCallback(async () => {
    try {
      const r = await fetch("/api/spots");
      if (r.ok) setSpots(await r.json());
    } catch { /* ignore network errors */ }
  }, []);

  useEffect(() => {
    if (step === "plate") return;
    fetchSpots();
    const id = setInterval(fetchSpots, 5000);
    return () => clearInterval(id);
  }, [step, fetchSpots]);

  useEffect(() => {
    const id = setInterval(() => setNow(new Date()), 1000);
    return () => clearInterval(id);
  }, []);

  const myRes = spots
    .map((s) => s.activeReservation ? { ...s.activeReservation, spotName: s.name } : null)
    .find((r) => r && r.licensePlate === plate && r.status === "ACTIVE" && new Date(r.endTime) > now) ?? null;

  function handleContinue() {
    const p = inputPlate.replace(/[^A-Z0-9-]/g, "");
    if (p.length < 4) { setError("Plaque invalide (min. 4 caractères)"); return; }
    setPlate(p);
    setStep("grid");
    setError("");
  }

  async function handlePay() {
    if (!selectedSpot) return;
    setLoading(true);
    setError("");
    try {
      const r = await fetch("/api/reservations", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          spotId: selectedSpot.id,
          licensePlate: plate,
          durationMinutes: duration.minutes,
          amountPaid: duration.price,
        }),
      });
      const data = await r.json();
      if (!r.ok) { setError(data.error ?? "Erreur"); return; }
      setConfirmed({ ...data, spotName: selectedSpot.name });
      await fetchSpots();
      setStep("success");
    } catch {
      setError("Erreur réseau, réessayez.");
    } finally {
      setLoading(false);
    }
  }

  // ── PLATE ──────────────────────────────────────────────────────────────────
  if (step === "plate") {
    return (
      <div className="min-h-screen bg-gray-950 flex items-center justify-center p-6">
        <div className="w-full max-w-sm space-y-8">
          <div className="text-center space-y-1">
            <div className="text-6xl mb-2">🅿️</div>
            <h1 className="text-3xl font-black text-white tracking-widest">PARKING</h1>
            <p className="text-blue-400 font-bold tracking-[0.3em] text-sm">INTELLIGENT</p>
          </div>
          <div className="bg-gray-800 rounded-2xl p-6 space-y-5 shadow-2xl">
            <p className="text-gray-400 text-xs uppercase tracking-widest text-center">
              Plaque d&apos;immatriculation
            </p>
            <input
              type="text"
              value={inputPlate}
              onChange={(e) => { setInputPlate(e.target.value.toUpperCase()); setError(""); }}
              onKeyDown={(e) => { if (e.key === "Enter") handleContinue(); }}
              placeholder="AB-123-CD"
              maxLength={12}
              autoFocus
              className="w-full bg-gray-700 text-white text-3xl font-mono text-center rounded-xl px-4 py-4 outline-none focus:ring-2 focus:ring-blue-500 placeholder:text-gray-600"
            />
            {error && <p className="text-red-400 text-sm text-center">{error}</p>}
            <button
              onClick={handleContinue}
              className="w-full bg-blue-600 hover:bg-blue-500 active:scale-95 text-white text-lg font-bold py-4 rounded-xl transition-all"
            >
              CONTINUER →
            </button>
          </div>
          <p className="text-gray-700 text-xs text-center">Équipe 154</p>
        </div>
      </div>
    );
  }

  // ── RESERVE ────────────────────────────────────────────────────────────────
  if (step === "reserve" && selectedSpot) {
    const endPreview = new Date(now.getTime() + duration.minutes * 60_000);
    return (
      <div className="min-h-screen bg-gray-950 flex items-center justify-center p-6">
        <div className="w-full max-w-sm space-y-4">
          <button
            onClick={() => { setStep("grid"); setError(""); }}
            className="text-gray-400 hover:text-white flex items-center gap-2 transition-colors"
          >
            ← Retour
          </button>
          <div className="bg-gray-800 rounded-2xl p-6 space-y-6 shadow-2xl">
            <h2 className="text-2xl font-bold text-white text-center">
              Réserver {selectedSpot.name}
            </h2>
            <div className="space-y-2">
              <p className="text-gray-400 text-xs uppercase tracking-widest">Durée</p>
              <div className="grid grid-cols-2 gap-3">
                {DURATIONS.map((d) => (
                  <button
                    key={d.minutes}
                    onClick={() => setDuration(d)}
                    className={`py-4 rounded-xl font-bold transition-all active:scale-95 ${
                      duration.minutes === d.minutes
                        ? "bg-blue-600 text-white ring-2 ring-blue-400"
                        : "bg-gray-700 text-gray-300 hover:bg-gray-600"
                    }`}
                  >
                    <div className="text-lg">{d.label}</div>
                    <div className="text-sm opacity-75">{d.price.toFixed(2)} €</div>
                  </button>
                ))}
              </div>
            </div>
            <div className="border-t border-gray-700 pt-4 space-y-2 text-sm">
              {([
                ["Plaque",        plate],
                ["Place",         selectedSpot.name],
                ["Durée",         duration.label],
                ["Fin de validité", endPreview.toLocaleTimeString("fr-FR", { hour: "2-digit", minute: "2-digit" })],
              ] as [string, string][]).map(([k, v]) => (
                <div key={k} className="flex justify-between text-gray-300">
                  <span>{k}</span>
                  <span className="font-bold text-white font-mono">{v}</span>
                </div>
              ))}
              <div className="flex justify-between text-xl font-black pt-2">
                <span className="text-gray-300">Total</span>
                <span className="text-green-400">{duration.price.toFixed(2)} €</span>
              </div>
            </div>
            {error && <p className="text-red-400 text-sm text-center">{error}</p>}
            <button
              onClick={handlePay}
              disabled={loading}
              className="w-full bg-green-600 hover:bg-green-500 disabled:opacity-50 active:scale-95 text-white text-lg font-bold py-4 rounded-xl transition-all"
            >
              {loading ? "TRAITEMENT..." : "PAYER MAINTENANT 💳"}
            </button>
          </div>
        </div>
      </div>
    );
  }

  // ── SUCCESS ────────────────────────────────────────────────────────────────
  if (step === "success") {
    const r = confirmed;
    return (
      <div className="min-h-screen bg-gray-950 flex items-center justify-center p-6">
        <div className="w-full max-w-sm space-y-6 text-center">
          <div className="text-7xl">✅</div>
          <h2 className="text-3xl font-black text-white">Réservation confirmée !</h2>
          {r && (
            <div className="bg-gray-800 rounded-2xl p-6 space-y-3 text-left shadow-2xl">
              {([
                ["Place",          r.spotName],
                ["Plaque",         r.licensePlate],
                ["Valide jusqu'à", new Date(r.endTime).toLocaleTimeString("fr-FR", { hour: "2-digit", minute: "2-digit" })],
                ["Montant payé",   `${r.amountPaid.toFixed(2)} €`],
              ] as [string, string][]).map(([k, v]) => (
                <div key={k} className="flex justify-between text-sm">
                  <span className="text-gray-400">{k}</span>
                  <span className="font-bold text-white">{v}</span>
                </div>
              ))}
            </div>
          )}
          <button
            onClick={() => setStep("grid")}
            className="w-full bg-blue-600 hover:bg-blue-500 active:scale-95 text-white text-lg font-bold py-4 rounded-xl transition-all"
          >
            RETOUR AU PARKING
          </button>
        </div>
      </div>
    );
  }

  // ── GRID ───────────────────────────────────────────────────────────────────
  const freeCount = spots.filter((s) => getStatus(s, now) === "LIBRE").length;

  return (
    <div className="min-h-screen bg-gray-950 flex flex-col">
      <header className="bg-gray-900 border-b border-gray-800 px-5 py-3 flex items-center justify-between">
        <div>
          <p className="text-white font-bold text-sm">🅿️ PARKING INTELLIGENT</p>
          <p className="text-gray-500 text-xs font-mono">{now.toLocaleTimeString("fr-FR")}</p>
        </div>
        <button onClick={() => { setStep("plate"); setInputPlate(""); }} className="text-right">
          <p className="text-gray-500 text-xs">Plaque</p>
          <p className="text-blue-400 font-mono font-bold text-sm hover:text-blue-300">{plate} ✎</p>
        </button>
      </header>

      {myRes && (
        <div className="bg-blue-950 border-b border-blue-800 px-5 py-2 flex items-center justify-between">
          <p className="text-blue-300 text-sm">
            <span className="font-bold">{myRes.spotName}</span> — votre réservation
          </p>
          <p className="text-white font-mono font-bold text-lg">{countdown(myRes.endTime, now)}</p>
        </div>
      )}

      <main className="flex-1 p-5 flex flex-col items-center justify-center gap-6">
        {spots.length === 0 ? (
          <p className="text-gray-600 text-sm">Chargement...</p>
        ) : (
          <>
            <div className="grid grid-cols-3 gap-3 w-full max-w-xs">
              {spots.map((spot) => {
                const st = getStatus(spot, now);
                const cfg = STATUS_CFG[st];
                const isClickable = st === "LIBRE";
                const isMySpot = spot.activeReservation?.licensePlate === plate;
                const res = spot.activeReservation;

                return (
                  <button
                    key={spot.id}
                    onClick={() => {
                      if (!isClickable) return;
                      setSelectedSpot(spot);
                      setDuration(DURATIONS[1]);
                      setError("");
                      setStep("reserve");
                    }}
                    disabled={!isClickable}
                    className={`rounded-2xl border-2 p-3 text-left transition-all ${cfg.bg} ${cfg.border} ${
                      isClickable ? "cursor-pointer active:scale-95 hover:brightness-125" : "cursor-default"
                    }`}
                  >
                    <p className="text-white font-bold text-base">{spot.name}</p>
                    <p className={`text-xs font-semibold mt-0.5 ${cfg.labelColor}`}>
                      {isMySpot && st === "EN_COURS" ? "MA PLACE" : cfg.label}
                    </p>
                    {res && (st === "EN_COURS" || st === "DEPASSE") && (
                      <p className="text-xs font-mono text-gray-400 mt-1">{countdown(res.endTime, now)}</p>
                    )}
                    {st === "DEPASSE" && res && res.fineAmount > 0 && (
                      <p className="text-xs text-orange-400 font-bold">+{res.fineAmount.toFixed(0)} €</p>
                    )}
                    {isClickable && (
                      <p className="text-xs text-gray-600 mt-1">Appuyez ici</p>
                    )}
                  </button>
                );
              })}
            </div>

            <div className="flex flex-wrap gap-x-4 gap-y-1 justify-center">
              {(["LIBRE", "RESERVE", "OCCUPE", "DEPASSE"] as SpotStatus[]).map((s) => (
                <div key={s} className="flex items-center gap-1.5">
                  <div className={`w-2 h-2 rounded-full ${STATUS_CFG[s].dot}`} />
                  <span className="text-gray-500 text-xs">{STATUS_CFG[s].label.charAt(0) + STATUS_CFG[s].label.slice(1).toLowerCase()}</span>
                </div>
              ))}
            </div>

            <p className="text-gray-600 text-xs">{freeCount} / {spots.length} places libres</p>
          </>
        )}
      </main>
    </div>
  );
}
