from __future__ import annotations

import math
import random
import struct
import wave
from pathlib import Path


SAMPLE_RATE = 48_000
DURATION_SECONDS = 0.95
SEED = 20260710

ROOT = Path(__file__).resolve().parents[2]
OUTPUT_PATH = ROOT / "Content" / "Audio" / "SFX" / "S_CoinGain_Rattle_01.wav"
SOURCE_NOTE_PATH = ROOT / "Content" / "Audio" / "SFX" / "S_CoinGain_Rattle_01_SOURCE.txt"


def equal_power_pan(pan: float) -> tuple[float, float]:
    pan = max(-1.0, min(1.0, pan))
    angle = (pan + 1.0) * math.pi * 0.25
    return math.cos(angle), math.sin(angle)


def add_metal_tick(
    left: list[float],
    right: list[float],
    rng: random.Random,
    start_seconds: float,
    base_frequency: float,
    amplitude: float,
    pan: float,
    duration: float,
    decay: float,
) -> None:
    start = int(start_seconds * SAMPLE_RATE)
    length = int(duration * SAMPLE_RATE)
    l_gain, r_gain = equal_power_pan(pan)

    # Inharmonic ratios create a small, bright metal-coin character.
    ratios = (1.0, 2.71, 3.86, 5.32, 7.1)
    levels = (1.0, 0.56, 0.34, 0.2, 0.09)
    phases = [rng.random() * math.tau for _ in ratios]

    for offset in range(length):
        index = start + offset
        if index >= len(left):
            break

        t = offset / SAMPLE_RATE
        attack = 1.0 - math.exp(-t / 0.0012)
        body_env = math.exp(-t / decay) * attack
        click_env = math.exp(-t / 0.0045)
        pitch_lift = 1.0 + 0.018 * math.exp(-t / 0.018)

        ring = 0.0
        for ratio, level, phase in zip(ratios, levels, phases):
            frequency = base_frequency * ratio * pitch_lift
            ring += level * math.sin(math.tau * frequency * t + phase)

        # Short synthetic impact noise. This is generated noise, not a sample.
        impact = (rng.random() * 2.0 - 1.0) * click_env
        sample = amplitude * ((ring * 0.32 * body_env) + (impact * 0.46 * click_env))

        left[index] += sample * l_gain
        right[index] += sample * r_gain


def add_sparkle_tail(
    left: list[float],
    right: list[float],
    rng: random.Random,
    start_seconds: float,
) -> None:
    for i, frequency in enumerate((1840.0, 2360.0, 3120.0)):
        add_metal_tick(
            left,
            right,
            rng,
            start_seconds + i * 0.018,
            frequency,
            0.095 - i * 0.018,
            (-0.25, 0.2, 0.0)[i],
            0.24,
            0.072,
        )


def soft_limit(value: float) -> float:
    return math.tanh(value * 1.35) / math.tanh(1.35)


def render() -> tuple[list[float], list[float]]:
    rng = random.Random(SEED)
    frame_count = int(DURATION_SECONDS * SAMPLE_RATE)
    left = [0.0] * frame_count
    right = [0.0] * frame_count

    t = 0.025
    for i in range(18):
        progress = i / 17.0
        base = 940.0 + progress * 860.0 + rng.uniform(-70.0, 90.0)
        amp = 0.12 + 0.045 * math.sin(progress * math.pi)
        pan = rng.uniform(-0.55, 0.55)
        add_metal_tick(
            left,
            right,
            rng,
            t,
            base,
            amp,
            pan,
            rng.uniform(0.13, 0.18),
            rng.uniform(0.035, 0.055),
        )
        t += rng.uniform(0.028, 0.043)

    # Extra quiet micro-clicks fill the spaces so it reads as a coin rattle,
    # not one isolated UI beep.
    for i in range(31):
        t = 0.018 + i * rng.uniform(0.014, 0.019)
        add_metal_tick(
            left,
            right,
            rng,
            t,
            rng.uniform(1500.0, 2800.0),
            rng.uniform(0.018, 0.032),
            rng.uniform(-0.65, 0.65),
            rng.uniform(0.045, 0.075),
            rng.uniform(0.013, 0.024),
        )

    add_sparkle_tail(left, right, rng, 0.70)

    # Fade the tail cleanly to avoid a click at the file boundary.
    fade_length = int(0.095 * SAMPLE_RATE)
    for i in range(fade_length):
        gain = 1.0 - (i / fade_length)
        index = frame_count - fade_length + i
        left[index] *= gain
        right[index] *= gain

    peak = max(max(abs(x) for x in left), max(abs(x) for x in right))
    normalize = 0.91 / peak if peak else 1.0
    for i in range(frame_count):
        left[i] = soft_limit(left[i] * normalize)
        right[i] = soft_limit(right[i] * normalize)

    return left, right


def write_wav(left: list[float], right: list[float]) -> None:
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUTPUT_PATH), "wb") as output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)

        frames = bytearray()
        for l_sample, r_sample in zip(left, right):
            l_int = int(max(-1.0, min(1.0, l_sample)) * 32767)
            r_int = int(max(-1.0, min(1.0, r_sample)) * 32767)
            frames.extend(struct.pack("<hh", l_int, r_int))
        output.writeframes(frames)


def write_source_note() -> None:
    SOURCE_NOTE_PATH.write_text(
        "\n".join(
            (
                "S_CoinGain_Rattle_01.wav",
                "Generated procedurally from Tools/Audio/generate_coin_gain_rattle.py.",
                "No external audio samples, sound libraries, open-source assets, or commercial stock assets were used.",
                "Format: 48 kHz, 16-bit PCM stereo WAV.",
                f"Deterministic seed: {SEED}.",
                "",
            )
        ),
        encoding="utf-8",
    )


def main() -> None:
    left, right = render()
    write_wav(left, right)
    write_source_note()
    print(f"Wrote {OUTPUT_PATH}")
    print(f"Wrote {SOURCE_NOTE_PATH}")


if __name__ == "__main__":
    main()
