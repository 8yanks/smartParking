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
  spot: { name: string };
};

type Spot = {
  id: number;
  name: string;
  is_occupied: boolean;
  activeReservation: { status: string; endTime: string } | null;
};

const PRESETS = [5, 10, 30, 60, 300];

export default function Admin() {
  const [reservations, setReservations] = useState<Reservation[]>([]);
  const [spots, setSpots] = useState<Spot[]>([]);
  const [spotId, setSpotId] = useState(1);
  const [plate, setPlate] = useState("TEST-001");
  const [duration, setDuration] = useState(5);
  const [amount, setAmount] = useState(0.01);
  const [now, setNow] = useState(() => new Date());
  const [busy, setBusy] = useState(false);
  const [flash, setFlash] = useState("");

  const fetchAll = useCallback(async () => {
    try {
      const [r1, r2] = await Promise.all([
        fetch("/api/admin/reservations"),
        fetch("/api/spots"),
      ]);
      if (r1.ok) setReservations(await r1.json());
      if (r2.ok) setSpots(await r2.json());
    } catch { /* ignore */ }
  }, []);

  useEffect(() => {
    fetchAll();
    const id = setInterval(fetchAll, 1500);
    return () => clearInterval(id);
  }, [fetchAll]);

  useEffect(() => {
    const id = setInterval(() => setNow(new Date()), 500);
    return () => clearInterval(id);
  }, []);

  function notify(msg: string) {
    setFlash(msg);
    setTimeout(() => setFlash(""), 2000);
  }

  async function createTest() {
    setBusy(true);
    try {
      const r = await fetch("/api/admin/reservations", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ spotId, licensePlate: plate, durationSeconds: duration, amountPaid: amount }),
      });
      const data = await r.json();
      if (!r.ok) notify(data.error ?? "Erreur");
      else notify(`✓ Réservation #${data.id} créée`);
      await fetchAll();
    } finally {
      setBusy(false);
    }
  }

  async function deleteRes(id: number) {
    setBusy(true);
    try {
      await fetch(`/api/admin/reservations/${id}`, { method: "DELETE" });
      notify(`✓ Réservation #${id} supprimée`);
      await fetchAll();
    } finally {
      setBusy(false);
    }
  }

  async function toggleSpot(id: number, occupied: boolean) {
    setBusy(true);
    try {
      await fetch(`/api/admin/spot/${id}`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ occupied }),
      });
      notify(`✓ Place ${id} → ${occupied ? "OCCUPÉE" : "LIBRE"}`);
      await fetchAll();
    } finally {
      setBusy(false);
    }
  }

  async function resetAll() {
    if (!confirm("Supprimer TOUTES les réservations et remettre toutes les places libres ?")) return;
    setBusy(true);
    try {
      await fetch("/api/admin/reset", { method: "POST" });
      notify("✓ Base réinitialisée");
      await fetchAll();
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="min-h-screen bg-gray-950 text-white p-4 sm:p-6">
      <div className="max-w-3xl mx-auto space-y-5">
        <header className="flex justify-between items-baseline border-b border-gray-800 pb-3">
          <div>
            <h1 className="text-2xl font-black tracking-wider">⚙️ ADMIN</h1>
            <p className="text-gray-500 text-xs">Tests & diagnostics</p>
          </div>
          <a href="/" className="text-blue-400 text-sm hover:underline">← Parking</a>
        </header>

        {flash && (
          <div className="bg-blue-900/40 border border-blue-700 rounded-lg px-4 py-2 text-sm text-blue-200">
            {flash}
          </div>
        )}

        {/* CREATE */}
        <section className="bg-gray-800 rounded-2xl p-5 space-y-4">
          <h2 className="font-bold text-lg">Créer une réservation de test</h2>
          <div className="grid grid-cols-2 gap-3">
            <label className="space-y-1 block">
              <span className="text-xs text-gray-400 uppercase">Place</span>
              <select value={spotId} onChange={(e) => setSpotId(Number(e.target.value))} className="w-full bg-gray-700 rounded-lg px-3 py-2 outline-none">
                {spots.length === 0 && <option value={1}>Place 1</option>}
                {spots.map((s) => <option key={s.id} value={s.id}>{s.name}</option>)}
              </select>
            </label>
            <label className="space-y-1 block">
              <span className="text-xs text-gray-400 uppercase">Plaque</span>
              <input type="text" value={plate} onChange={(e) => setPlate(e.target.value.toUpperCase())} className="w-full bg-gray-700 rounded-lg px-3 py-2 font-mono outline-none" />
            </label>
            <label className="space-y-1 block">
              <span className="text-xs text-gray-400 uppercase">Durée (secondes)</span>
              <input type="number" value={duration} onChange={(e) => setDuration(Number(e.target.value))} min={1} className="w-full bg-gray-700 rounded-lg px-3 py-2 outline-none" />
            </label>
            <label className="space-y-1 block">
              <span className="text-xs text-gray-400 uppercase">Montant (€)</span>
              <input type="number" value={amount} onChange={(e) => setAmount(Number(e.target.value))} min={0} step={0.01} className="w-full bg-gray-700 rounded-lg px-3 py-2 outline-none" />
            </label>
          </div>
          <div className="flex flex-wrap gap-2">
            {PRESETS.map((p) => (
              <button key={p} onClick={() => setDuration(p)} className={`text-xs px-3 py-1.5 rounded-lg font-bold ${duration === p ? "bg-blue-600" : "bg-gray-700 hover:bg-gray-600"}`}>
                {p < 60 ? `${p}s` : `${p / 60}min`}
              </button>
            ))}
          </div>
          <button onClick={createTest} disabled={busy} className="w-full bg-blue-600 hover:bg-blue-500 disabled:opacity-50 active:scale-95 py-3 rounded-lg font-bold transition-all">
            CRÉER LA RÉSERVATION
          </button>
        </section>

        {/* SPOTS */}
        <section className="bg-gray-800 rounded-2xl p-5 space-y-3">
          <h2 className="font-bold text-lg">État des capteurs (override manuel)</h2>
          <p className="text-xs text-gray-500">
            ⚠ Les ESP32 réécrivent l&apos;état toutes les ~5s. L&apos;override est temporaire.
          </p>
          <div className="grid grid-cols-3 gap-2">
            {spots.map((s) => (
              <div key={s.id} className={`rounded-lg p-3 border ${s.is_occupied ? "bg-red-900/30 border-red-700" : "bg-green-900/20 border-green-700"}`}>
                <p className="font-bold text-sm">{s.name}</p>
                <p className={`text-xs mb-2 ${s.is_occupied ? "text-red-300" : "text-green-300"}`}>
                  {s.is_occupied ? "OCCUPÉ" : "LIBRE"}
                </p>
                <button
                  onClick={() => toggleSpot(s.id, !s.is_occupied)}
                  disabled={busy}
                  className="w-full bg-gray-700 hover:bg-gray-600 disabled:opacity-50 text-xs py-1 rounded font-mono"
                >
                  Toggle
                </button>
              </div>
            ))}
          </div>
        </section>

        {/* RESERVATIONS */}
        <section className="bg-gray-800 rounded-2xl p-5 space-y-3">
          <div className="flex justify-between items-center">
            <h2 className="font-bold text-lg">Réservations ({reservations.length})</h2>
            <button onClick={resetAll} disabled={busy} className="bg-red-900 hover:bg-red-800 disabled:opacity-50 text-red-200 text-xs font-bold px-3 py-1.5 rounded-lg">
              ⚠ RESET TOUT
            </button>
          </div>
          {reservations.length === 0 ? (
            <p className="text-gray-500 text-sm text-center py-4">Aucune réservation.</p>
          ) : (
            <ul className="space-y-2 max-h-96 overflow-y-auto">
              {reservations.map((r) => {
                const remaining = new Date(r.endTime).getTime() - now.getTime();
                const isActive = r.status === "ACTIVE";
                const isExpired = r.status === "EXPIRED";
                const isCancelled = r.status === "CANCELLED";
                const statusColor = isExpired
                  ? "bg-orange-900/30 border-orange-700"
                  : isCancelled
                  ? "bg-gray-700/30 border-gray-600 opacity-60"
                  : isActive && remaining > 0
                  ? "bg-blue-900/30 border-blue-700"
                  : "bg-gray-700/40 border-gray-600";
                return (
                  <li key={r.id} className={`rounded-lg p-3 text-sm border ${statusColor}`}>
                    <div className="flex justify-between items-start gap-2">
                      <div className="flex-1 min-w-0">
                        <p className="font-bold">
                          #{r.id} • {r.spot.name} • <span className="font-mono text-blue-300">{r.licensePlate}</span>
                        </p>
                        <p className="text-xs text-gray-400 mt-0.5">
                          <span className={`font-bold ${isExpired ? "text-orange-400" : isActive ? "text-blue-400" : "text-gray-500"}`}>{r.status}</span>
                          {" • "}
                          {isActive && remaining > 0
                            ? `fin dans ${Math.max(0, Math.floor(remaining / 1000))}s`
                            : `fin ${new Date(r.endTime).toLocaleTimeString("fr-FR")}`}
                        </p>
                        <p className="text-xs text-gray-500">
                          payé {r.amountPaid.toFixed(2)}€
                          {r.fineAmount > 0 && <span className="text-orange-400 font-bold"> + amende {r.fineAmount.toFixed(2)}€</span>}
                        </p>
                      </div>
                      <button onClick={() => deleteRes(r.id)} disabled={busy} className="text-red-400 hover:text-red-300 text-xs disabled:opacity-50">
                        ✕
                      </button>
                    </div>
                  </li>
                );
              })}
            </ul>
          )}
        </section>
      </div>
    </div>
  );
}
