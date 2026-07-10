from __future__ import annotations

import math
import random
import struct
import wave
from dataclasses import dataclass
from pathlib import Path


SAMPLE_RATE = 48_000
SEED = 20260710

ROOT = Path(__file__).resolve().parents[2]
OUTPUT_DIR = ROOT / "Content" / "Audio" / "SFX"
SOURCE_NOTE_PATH = OUTPUT_DIR / "S_CoinGain_Rattle_AltPack_SOURCE.txt"


@dataclass(frozen=True)
class Variant:
    index: int
    description: str
    duration: float
    event_count: int
    interval_min: float
    interval_max: float
    pitch_min: float
    pitch_max: float
    amplitude: float
    tail: float
    density: float
    damping: float
    low_body: float
    stereo_width: float


VARIANTS = (
    Variant(1, "dry mid-weight coin count, restrained UI reward", 0.70, 13, 0.030, 0.044, 520, 980, 0.16, 0.18, 0.60, 0.070, 0.38, 0.34),
    Variant(2, "heavier payout rattle with grounded final settle", 0.86, 15, 0.036, 0.055, 410, 820, 0.18, 0.24, 0.52, 0.090, 0.54, 0.28),
    Variant(3, "muted leather pouch coins, soft but continuous", 0.82, 17, 0.028, 0.042, 470, 920, 0.13, 0.16, 0.76, 0.050, 0.48, 0.42),
    Variant(4, "dense shop-register coin run, less sparkly", 0.94, 22, 0.024, 0.036, 560, 1120, 0.12, 0.20, 0.92, 0.045, 0.28, 0.46),
    Variant(5, "low bronze cascade for rare reward pickup", 0.90, 16, 0.034, 0.050, 340, 720, 0.19, 0.28, 0.58, 0.105, 0.66, 0.30),
    Variant(6, "compact premium UI tally, quick and sober", 0.58, 11, 0.026, 0.038, 610, 1180, 0.14, 0.12, 0.68, 0.048, 0.30, 0.32),
    Variant(7, "small stack spill with a thicker last coin", 0.78, 14, 0.032, 0.048, 450, 900, 0.15, 0.22, 0.62, 0.078, 0.52, 0.38),
    Variant(8, "slow weighted treasure count, deliberate rhythm", 1.04, 18, 0.040, 0.064, 380, 760, 0.17, 0.28, 0.46, 0.096, 0.70, 0.24),
    Variant(9, "clean reward roll, audible rattle without comedy brightness", 0.74, 16, 0.026, 0.039, 520, 1040, 0.13, 0.16, 0.82, 0.052, 0.36, 0.44),
    Variant(10, "soft banked tally, calm confirmation with many clicks", 0.68, 18, 0.020, 0.032, 480, 980, 0.105, 0.18, 1.00, 0.040, 0.26, 0.36),
)


def equal_power_pan(pan: float) -> tuple[float, float]:
    pan = max(-1.0, min(1.0, pan))
    angle = (pan + 1.0) * math.pi * 0.25
    return math.cos(angle), math.sin(angle)


def add_coin_hit(
    left: list[float],
    right: list[float],
    rng: random.Random,
    start_seconds: float,
    base_frequency: float,
    amplitude: float,
    pan: float,
    duration: float,
    damping: float,
    low_body: float,
    muted: float,
) -> None:
    start = int(start_seconds * SAMPLE_RATE)
    length = int(duration * SAMPLE_RATE)
    l_gain, r_gain = equal_power_pan(pan)

    ratios = (1.0, 1.37, 2.04, 2.91, 4.16, 5.72)
    levels = (1.0, 0.58, 0.36, 0.23, 0.13, 0.06)
    phases = [rng.random() * math.tau for _ in ratios]
    scrape_phase = rng.random() * math.tau

    for offset in range(length):
        index = start + offset
        if index >= len(left):
            break

        t = offset / SAMPLE_RATE
        attack = 1.0 - math.exp(-t / 0.0018)
        main_env = math.exp(-t / damping) * attack
        click_env = math.exp(-t / 0.0065)
        body_env = math.exp(-t / 0.045) * attack
        bend = 1.0 + 0.010 * math.exp(-t / 0.020)

        ring = 0.0
        for ratio, level, phase in zip(ratios, levels, phases):
            high_tame = 1.0 / (1.0 + muted * max(0.0, ratio - 2.0))
            frequency = base_frequency * ratio * bend
            ring += level * high_tame * math.sin(math.tau * frequency * t + phase)

        body = low_body * math.sin(math.tau * (base_frequency * 0.42) * t + phases[0]) * body_env
        scrape = 0.18 * math.sin(math.tau * (base_frequency * 0.18) * t + scrape_phase)
        impact = (rng.random() * 2.0 - 1.0 + scrape) * click_env
        sample = amplitude * ((ring * 0.34 * main_env) + (impact * 0.22 * click_env) + body)

        left[index] += sample * l_gain
        right[index] += sample * r_gain


def one_pole_lowpass(samples: list[float], cutoff_hz: float) -> None:
    rc = 1.0 / (math.tau * cutoff_hz)
    dt = 1.0 / SAMPLE_RATE
    alpha = dt / (rc + dt)
    previous = 0.0

    for i, sample in enumerate(samples):
        previous += alpha * (sample - previous)
        samples[i] = previous


def add_settle(left: list[float], right: list[float], rng: random.Random, spec: Variant, start: float) -> None:
    for i in range(3):
        add_coin_hit(
            left,
            right,
            rng,
            start + i * rng.uniform(0.030, 0.050),
            rng.uniform(spec.pitch_min * 0.75, spec.pitch_max * 0.92),
            spec.amplitude * (0.50 - i * 0.09),
            rng.uniform(-spec.stereo_width, spec.stereo_width),
            spec.tail + 0.04,
            spec.damping * 1.35,
            spec.low_body * 1.18,
            1.0 - spec.density * 0.25,
        )


def soft_limit(value: float) -> float:
    return math.tanh(value * 1.18) / math.tanh(1.18)


def render_variant(spec: Variant) -> tuple[list[float], list[float]]:
    rng = random.Random(SEED + spec.index * 101)
    frame_count = int(spec.duration * SAMPLE_RATE)
    left = [0.0] * frame_count
    right = [0.0] * frame_count

    current_time = 0.022
    for i in range(spec.event_count):
        progress = i / max(1, spec.event_count - 1)
        pitch_bias = 0.88 + progress * 0.18 + rng.uniform(-0.06, 0.07)
        amplitude_bias = 0.82 + 0.22 * math.sin(progress * math.pi)
        add_coin_hit(
            left,
            right,
            rng,
            current_time + rng.uniform(-0.006, 0.008),
            rng.uniform(spec.pitch_min, spec.pitch_max) * pitch_bias,
            spec.amplitude * amplitude_bias * rng.uniform(0.84, 1.08),
            rng.uniform(-spec.stereo_width, spec.stereo_width),
            rng.uniform(spec.tail * 0.75, spec.tail * 1.15),
            spec.damping * rng.uniform(0.85, 1.18),
            spec.low_body * rng.uniform(0.78, 1.15),
            1.0 - spec.density * 0.22,
        )
        current_time += rng.uniform(spec.interval_min, spec.interval_max)

    filler_count = int(spec.event_count * spec.density)
    for _ in range(filler_count):
        add_coin_hit(
            left,
            right,
            rng,
            rng.uniform(0.026, max(0.04, min(spec.duration - 0.20, current_time + 0.03))),
            rng.uniform(spec.pitch_min * 0.9, spec.pitch_max * 1.18),
            spec.amplitude * rng.uniform(0.18, 0.36),
            rng.uniform(-spec.stereo_width, spec.stereo_width),
            rng.uniform(0.055, 0.10),
            spec.damping * rng.uniform(0.55, 0.82),
            spec.low_body * rng.uniform(0.25, 0.55),
            1.30,
        )

    settle_time = min(spec.duration - 0.18, current_time + 0.020)
    add_settle(left, right, rng, spec, max(0.04, settle_time))

    cutoff = 4_800 + spec.density * 2_200
    one_pole_lowpass(left, cutoff)
    one_pole_lowpass(right, cutoff)

    fade_length = int(0.080 * SAMPLE_RATE)
    for i in range(fade_length):
        gain = 1.0 - (i / fade_length)
        index = frame_count - fade_length + i
        left[index] *= gain
        right[index] *= gain

    peak = max(max(abs(x) for x in left), max(abs(x) for x in right))
    normalize = 0.86 / peak if peak else 1.0
    for i in range(frame_count):
        left[i] = soft_limit(left[i] * normalize)
        right[i] = soft_limit(right[i] * normalize)

    return left, right


def write_wav(path: Path, left: list[float], right: list[float]) -> None:
    with wave.open(str(path), "wb") as output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)

        frames = bytearray()
        for l_sample, r_sample in zip(left, right):
            l_int = int(max(-1.0, min(1.0, l_sample)) * 32767)
            r_int = int(max(-1.0, min(1.0, r_sample)) * 32767)
            frames.extend(struct.pack("<hh", l_int, r_int))
        output.writeframes(frames)


def output_name(spec: Variant) -> str:
    return f"S_CoinGain_Rattle_Alt_{spec.index:02d}"


def write_source_note() -> None:
    lines = [
        "S_CoinGain_Rattle_Alt_01.wav through S_CoinGain_Rattle_Alt_10.wav",
        "Generated procedurally from Tools/Audio/generate_coin_gain_rattle_alt_pack.py.",
        "No external audio samples, sound libraries, open-source assets, or commercial stock assets were used.",
        "Format: 48 kHz, 16-bit PCM stereo WAV.",
        f"Deterministic seed base: {SEED}.",
        "",
        "Variant notes:",
    ]

    for spec in VARIANTS:
        lines.append(f"- {output_name(spec)}: {spec.description}")

    SOURCE_NOTE_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for spec in VARIANTS:
        left, right = render_variant(spec)
        path = OUTPUT_DIR / f"{output_name(spec)}.wav"
        write_wav(path, left, right)
        print(f"Wrote {path}")

    write_source_note()
    print(f"Wrote {SOURCE_NOTE_PATH}")


if __name__ == "__main__":
    main()
