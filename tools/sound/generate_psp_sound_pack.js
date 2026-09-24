// The PSP's sound pack: every clip as raw PCM, indexed by clip id, streamed
// from the memory stick.
//
// Same file list and order as generate_sound_ids.js (the id is the index).
// An .aifc stands for its source WAV; an .ins bank names its WAV with use()
// and loops.
//
// Layout, little endian:
//   u32 count
//   count x { u32 offset, u32 sampleCount, u32 loopEnd }  (loopEnd 0 = one shot)
//   PCM16 mono 22050Hz data, each clip at a 2KB boundary

const fs = require("fs");
const path = require("path");
const util = require("util");

const ALIGN = 2048;

// Peak for menu sounds, as a fraction of full scale: they are mixed quiet
// (14%) and get lost on the PSP speaker.
const UI_SOUND_PEAK = 0.5;

// Peak for music, which is mixed quiet too and plays at the music volume.
const MUSIC_PEAK = 0.9;

function soundPeak(file) {
    if (file.startsWith("ui/")) {
        return UI_SOUND_PEAK;
    }

    if (file.startsWith("music/") || file.startsWith("ambient/music/")) {
        return MUSIC_PEAK;
    }

    return 0;
}

function raiseToPeak(data, peak) {
    const samples = new Int16Array(data.buffer.slice(data.byteOffset, data.byteOffset + data.length));
    let loudest = 1;

    for (const sample of samples) {
        loudest = Math.max(loudest, Math.abs(sample));
    }

    const gain = (peak * 32767) / loudest;

    if (gain <= 1) {
        return data;
    }

    for (let i = 0; i < samples.length; ++i) {
        samples[i] = Math.max(-32768, Math.min(32767, Math.round(samples[i] * gain)));
    }

    return Buffer.from(samples.buffer);
}

function readWav(file) {
    const buffer = fs.readFileSync(file);
    let format = null;
    let offset = 12;

    while (offset + 8 <= buffer.length) {
        const id = buffer.toString("ascii", offset, offset + 4);
        const size = buffer.readUInt32LE(offset + 4);

        if (id === "fmt ") {
            format = {
                encoding: buffer.readUInt16LE(offset + 8),
                channels: buffer.readUInt16LE(offset + 10),
                rate: buffer.readUInt32LE(offset + 12),
                bits: buffer.readUInt16LE(offset + 22),
            };
        } else if (id === "data") {
            if (!format || format.encoding !== 1 || format.channels !== 1 ||
                format.rate !== 22050 || format.bits !== 16) {
                throw new Error(`${file} is not PCM16 mono 22050Hz: ${JSON.stringify(format)}`);
            }

            return buffer.subarray(offset + 8, offset + 8 + size);
        }

        offset += 8 + size + (size & 1);
    }

    throw new Error(`${file} has no data chunk`);
}

function resolveInput(file, pakModifiedSoundDir) {
    if (file.endsWith(".ins")) {
        const text = fs.readFileSync(file, "utf8");
        const use = text.match(/use\("([^"]+)"\)/);
        const loopEnd = text.match(/loopEnd\s*=\s*(-?\d+)/);

        return {
            wav: path.resolve(path.dirname(file), use[1]),
            loopEnd: loopEnd ? parseInt(loopEnd[1]) : -1,
        };
    }

    return {
        wav: path.join(pakModifiedSoundDir, file.replace(/\.aifc$/, ".wav")),
        loopEnd: 0,
    };
}

const { values, positionals } = util.parseArgs({
    options: {
        "out": { type: "string" },
        "pak-modified-sound-dir": { type: "string" },
    },
    allowPositionals: true
});

const clips = positionals.map(file => {
    const { wav, loopEnd } = resolveInput(file, values["pak-modified-sound-dir"]);
    const raw = readWav(wav);
    const peak = soundPeak(file);
    const data = peak ? raiseToPeak(raw, peak) : raw;
    const sampleCount = data.length / 2;

    return {
        data,
        sampleCount,
        // -1 loops the whole clip, as sfz2n64 reads it
        loopEnd: loopEnd < 0 ? sampleCount : Math.min(loopEnd, sampleCount),
    };
});

const header = Buffer.alloc(4 + clips.length * 12);
header.writeUInt32LE(clips.length, 0);

const parts = [header];
let offset = Math.ceil(header.length / ALIGN) * ALIGN;
parts.push(Buffer.alloc(offset - header.length));

clips.forEach((clip, i) => {
    header.writeUInt32LE(offset, 4 + i * 12);
    header.writeUInt32LE(clip.sampleCount, 8 + i * 12);
    header.writeUInt32LE(clip.loopEnd, 12 + i * 12);

    const padded = Math.ceil(clip.data.length / ALIGN) * ALIGN;
    parts.push(clip.data, Buffer.alloc(padded - clip.data.length));
    offset += padded;
});

fs.writeFileSync(values["out"], Buffer.concat(parts));
