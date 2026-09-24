
#include "TextureDefinition.h"
#include "../FileUtils.h"
#include "../math/LeastSquares.h"

#include <iomanip>
#include <algorithm>
#include <map>
#include <iomanip>
#include <assimp/vector3.h>
#include <assimp/vector3.inl>

#include "CImgu8.h"

DataChunkStream::DataChunkStream() :
    mCurrentBufferPos(0),
    mCurrentBuffer(0) {}

void DataChunkStream::WriteBytes(const char* data, int byteCount) {
    for (int i = 0; i < byteCount; ++i) {
        WriteBits(data[i], 8);
    }
}

void DataChunkStream::WriteBits(int from, int bitCount) {
    if (!bitCount) {
        return;
    } else if (bitCount + mCurrentBufferPos > 64) {
        int firstChunkSize = 64 - mCurrentBufferPos;
        int secondChunkSize = bitCount - firstChunkSize;

        WriteBits(from >> secondChunkSize, firstChunkSize);
        FlushBuffer();
        WriteBits(from, secondChunkSize);
    } else {
        uint64_t mask = ~(~(uint64_t)0 << bitCount);
        mCurrentBuffer |= (from & mask) << (64 - mCurrentBufferPos - bitCount);
        mCurrentBufferPos += bitCount;
    }
}

const std::vector<uint64_t>& DataChunkStream::GetData() {
    FlushBuffer();
    return mData;
}

void DataChunkStream::FlushBuffer() {
    if (mCurrentBufferPos == 0) {
        return;
    }

    mData.push_back(mCurrentBuffer);

    mCurrentBufferPos = 0;
    mCurrentBuffer = 0;
}

PixelRGBAu8::PixelRGBAu8() : r(0), g(0), b(0), a(0) {}
PixelRGBAu8::PixelRGBAu8(uint8_t rVal, uint8_t gVal, uint8_t bVal, uint8_t aVal) : 
    r(rVal), g(gVal), b(bVal), a(aVal) {}

bool PixelRGBAu8::operator==(const PixelRGBAu8& other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool PixelRGBAu8::operator<(const PixelRGBAu8& other) const {
    return std::tie(r, g, b) < std::tie(other.r, other.g, other.b);
}

bool PixelRGBAu8::WriteToStream(DataChunkStream& output, G_IM_SIZ size) const {
    switch (size) {
        case G_IM_SIZ::G_IM_SIZ_32b:
            output.WriteBytes((const char*)this, sizeof(PixelRGBAu8));
            return true;
        case G_IM_SIZ::G_IM_SIZ_16b:
            output.WriteBits(r >> 3, 5);
            output.WriteBits(g >> 3, 5);
            output.WriteBits(b >> 3, 5);
            output.WriteBits(a >> 7, 1);
            return true;
        default:
            return false;

    }
}

PixelIu8::PixelIu8(uint8_t i) : i(i) {}

bool PixelIu8::WriteToStream(DataChunkStream& output, G_IM_SIZ size) const {
    switch (size) {
        case G_IM_SIZ::G_IM_SIZ_8b:
            output.WriteBits(i, 8);
            return true;
        case G_IM_SIZ::G_IM_SIZ_4b:
            output.WriteBits(i >> 4, 4);
            return true;
        default:
            return false;
    }
}

PixelIAu8::PixelIAu8(uint8_t i, uint8_t a) : i(i), a(a) {}

bool PixelIAu8::WriteToStream(DataChunkStream& output, G_IM_SIZ size) const {
    switch (size) {
        case G_IM_SIZ::G_IM_SIZ_16b:
            output.WriteBits(i, 8);
            output.WriteBits(a, 8);
            return true;
        case G_IM_SIZ::G_IM_SIZ_8b:
            output.WriteBits(i >> 4, 4);
            output.WriteBits(a >> 4, 4);
            return true;
        case G_IM_SIZ::G_IM_SIZ_4b:
            output.WriteBits(i >> 5, 3);
            output.WriteBits(a >> 7, 1);
            return true;
        default:
            return false;
    }
}

struct PixelRGBAu8 readRGBAPixel(cimg_library::CImg<unsigned char>& input, int x, int y) {
    struct PixelRGBAu8 result;

    result.r = 0;
    result.g = 0;
    result.b = 1;
    result.a = 0xFF;
    
    switch (input.spectrum()) {
        case 4:
            result.a = input(x, y, 0, 3);
        case 3:
            result.r = input(x, y, 0, 0);
            result.g = input(x, y, 0, 1);
            result.b = input(x, y, 0, 2);
            break;
        case 2:
            result.a = input(x, y, 0, 1);
        case 1:
            result.r = input(x, y, 0, 0);
            result.g = input(x, y, 0, 0);
            result.b = input(x, y, 0, 0);
            break;
    }
    
    return result;
}

struct PixelIu8 readIPixel(cimg_library::CImg<unsigned char>& input, int x, int y) {
    switch (input.spectrum()) {
        case 4:
        case 3:
            return PixelIu8((
                input(x, y, 0, 0) * 85 +
                input(x, y, 0, 1) * 86 +
                input(x, y, 0, 2) * 85
            ) >> 8);
        case 2:
        case 1:
            return PixelIu8(input(x, y, 0, 0));
    }

    return PixelIu8(0);
}

struct PixelIAu8 readIAPixel(cimg_library::CImg<unsigned char>& input, int x, int y) {
    uint8_t alpha = 0xFF;

    switch (input.spectrum()) {
        case 4:
            alpha = input(x, y, 0, 3);
        case 3:
            return PixelIAu8((
                input(x, y, 0, 0) * 85 +
                input(x, y, 0, 1) * 86 +
                input(x, y, 0, 2) * 85
            ) >> 8, alpha);
        case 2:
            alpha = input(x, y, 0, 1);
        case 1:
            return PixelIAu8(input(x, y, 0, 0), alpha);
    }

    return PixelIAu8(0, alpha);
}

void writeRGBAPixel(cimg_library::CImg<unsigned char>& input, int x, int y, struct PixelRGBAu8 value) {
    switch (input.spectrum()) {
        case 4:
            input(x, y, 0, 3) = value.a;
        case 3:
            input(x, y, 0, 0) = value.r;
            input(x, y, 0, 1) = value.g;
            input(x, y, 0, 2) = value.b;
            break;
        case 2:
            input(x, y, 0, 1) = value.a;
        case 1:
            input(x, y, 0, 0) = value.r;
            break;
    }
}

void writeIAPixel(cimg_library::CImg<unsigned char>& input, int x, int y, struct PixelIAu8 value) {
    switch (input.spectrum()) {
        case 4:
            input(x, y, 0, 3) = value.a;
        case 3:
            input(x, y, 0, 0) = value.i;
            input(x, y, 0, 1) = value.i;
            input(x, y, 0, 2) = value.i;
            break;
        case 2:
            input(x, y, 0, 1) = value.a;
        case 1:
            input(x, y, 0, 0) = value.i;
            break;
    }
}

bool convertPixel(cimg_library::CImg<unsigned char>& input, int x, int y, DataChunkStream& output, G_IM_FMT fmt, G_IM_SIZ siz, const std::shared_ptr<PaletteDefinition>& palette) {
    switch (fmt) {
        case G_IM_FMT::G_IM_FMT_RGBA: {
            PixelRGBAu8 pixel = readRGBAPixel(input, x, y);
            return pixel.WriteToStream(output, siz);
        }
        case G_IM_FMT::G_IM_FMT_I: {
            PixelIu8 pixel = readIPixel(input, x, y);
            return pixel.WriteToStream(output, siz);
        }
        case G_IM_FMT::G_IM_FMT_IA: {
            PixelIAu8 pixel = readIAPixel(input, x, y);
            return pixel.WriteToStream(output, siz);
        }
        case G_IM_FMT::G_IM_FMT_CI: {
            PixelIu8 pixel = palette ? palette->FindIndex(readRGBAPixel(input, x, y)) : readIPixel(input, x, y);
            if (siz == G_IM_SIZ::G_IM_SIZ_4b) {
                // WriteToStream() chops off bottom 4 bits, which is fine when
                // writing out actual intensity values. But palette indices
                // must be preserved, so shift left to counteract truncation.
                pixel.i <<= 4;
            }

            return pixel.WriteToStream(output, siz);
        }
        default:
            return false;
    }
}

const char* gFormatShortName[] = {
    "rgba",
    "yuv",
    "ci",
    "i",
    "ia",
};

const char* gSizeName[] = {
    "4b",
    "8b",
    "16b",
    "32b",
};

uint8_t interpolateGrayscale(int min, int max, uint8_t input) {
    if (input <= min) {
        return 0;
    }

    int result = 0x100 * (input - min + 1) / (max - min + 1) - 1;

    if (result > 0xFF) {
        return 0xFF;
    }

    return result;
}

uint8_t floatToByte(float input) {
    int result = (int)(input + 0.5f);

    if (result < 0) {
        return 0;
    }

    if (result > 0xFF) {
        return 0xFF;
    }

    return result;
}

void applyTwoToneEffect(cimg_library::CImg<unsigned char>& input, PixelRGBAu8& maxColor, PixelRGBAu8& minColor) {
    LinearLeastSquares r;
    LinearLeastSquares g;
    LinearLeastSquares b;
    LinearLeastSquares a;

    int minGray = 0xFF;
    int maxGray = 0;

    int minAlpha = 0xFF;
    int maxAlpha = 0;

    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelRGBAu8 colorValue = readRGBAPixel(input, x, y);
            PixelIAu8 grayScaleValue = readIAPixel(input, x, y);

            r.AddDataPoint(grayScaleValue.i, colorValue.r);
            g.AddDataPoint(grayScaleValue.i, colorValue.g);
            b.AddDataPoint(grayScaleValue.i, colorValue.b);
            a.AddDataPoint(grayScaleValue.a, colorValue.a);

            minGray = std::min((int)grayScaleValue.i, minGray);
            maxGray = std::max((int)grayScaleValue.i, maxGray);
            minAlpha = std::min((int)grayScaleValue.i, minAlpha);
            maxAlpha = std::max((int)grayScaleValue.i, maxAlpha);
        }
    }


    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelIAu8 grayScaleValue = readIAPixel(input, x, y);
            grayScaleValue.i = interpolateGrayscale(minGray, maxGray, grayScaleValue.i);
            grayScaleValue.a = interpolateGrayscale(minAlpha, maxAlpha, grayScaleValue.a);
            writeIAPixel(input, x, y, grayScaleValue);
        }
    }

    maxColor.r = floatToByte(r.PredictY(maxGray));
    maxColor.g = floatToByte(g.PredictY(maxGray));
    maxColor.b = floatToByte(b.PredictY(maxGray));
    maxColor.a = floatToByte(a.PredictY(maxAlpha));

    minColor.r = floatToByte(r.PredictY(minGray));
    minColor.g = floatToByte(g.PredictY(minGray));
    minColor.b = floatToByte(b.PredictY(minGray));
    minColor.a = floatToByte(a.PredictY(minAlpha));
}

#define NORMAL_45_STEEPNESS 16

void calculateNormalMap(cimg_library::CImg<unsigned char>& input) {
    cimg_library::CImg<unsigned char> result(input.width(), input.height(), 1, 3);

    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelIAu8 colorValue = readIAPixel(input, x, y);
            PixelIAu8 nextX = readIAPixel(input, (x + 1) % input.width(), y);
            PixelIAu8 nextY = readIAPixel(input, x, (y + 1) % input.height());
            
            aiVector3D xDir(NORMAL_45_STEEPNESS, 0, (nextX.i - colorValue.i) * colorValue.a * (1.0f / 256.0f));
            aiVector3D yDir(0, NORMAL_45_STEEPNESS, (nextY.i - colorValue.i) * colorValue.a * (1.0f / 256.0f));

            aiVector3D normal = xDir ^ yDir;
            normal.Normalize();

            writeRGBAPixel(result, x, y, PixelRGBAu8(
                (uint8_t)(127.0f * normal.x + 127.0f),
                (uint8_t)(127.0f * normal.y + 127.0f),
                (uint8_t)(127.0f * normal.z + 127.0f),
                255
            ));
        }
    }

    input = result;
}

void invertImage(cimg_library::CImg<unsigned char>& input) {
    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelRGBAu8 colorValue = readRGBAPixel(input, x, y);
            writeRGBAPixel(input, x, y, PixelRGBAu8(0xFF - colorValue.r, 0xFF - colorValue.g, 0xFF - colorValue.b, colorValue.a));
        }
    }
}

void selectChannel(cimg_library::CImg<unsigned char>& input, TextureDefinitionEffect effects) {
    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelRGBAu8 colorValue = readRGBAPixel(input, x, y);

            if ((int)effects & (int)TextureDefinitionEffect::SelectR) {
                writeIAPixel(input, x, y, PixelIAu8(colorValue.r, colorValue.a));
            } else if ((int)effects & (int)TextureDefinitionEffect::SelectG) {
                writeIAPixel(input, x, y, PixelIAu8(colorValue.g, colorValue.a));
            } else if ((int)effects & (int)TextureDefinitionEffect::SelectB) {
                writeIAPixel(input, x, y, PixelIAu8(colorValue.b, colorValue.a));
            }
        }
    }
}

int getTexelSwapMask(G_IM_SIZ siz) {
    switch (siz) {
        case G_IM_SIZ::G_IM_SIZ_4b:
            return 8;
        case G_IM_SIZ::G_IM_SIZ_8b:
            return 4;
        case G_IM_SIZ::G_IM_SIZ_16b:
        case G_IM_SIZ::G_IM_SIZ_32b:
            return 2;
        default:
            return 0;
    }
}

PaletteDefinition::PaletteDefinition(const std::string& filename):
    mName(getBaseName(replaceExtension(filename, "")) + "_tlut") {
    cimg_library::CImg<unsigned char> imageData(filename.c_str());

    std::set<PixelRGBAu8> uniqueColors;
    
    for (int y = 0; y < imageData.height(); ++y) {
        for (int x = 0; x < imageData.width(); ++x) {
            PixelRGBAu8 colorValue = readRGBAPixel(imageData, x, y);
            uniqueColors.insert(colorValue);
        }
    }

    DataChunkStream dataStream;

    for (auto& colorValue : uniqueColors) {
        mColors.push_back(colorValue);

        colorValue.WriteToStream(dataStream, G_IM_SIZ::G_IM_SIZ_16b);
    }

    auto data = dataStream.GetData();
    mData.resize(data.size());

    std::copy(data.begin(), data.end(), mData.begin());
}

PixelIu8 PaletteDefinition::FindIndex(PixelRGBAu8 color) const {
    unsigned result = 0;
    unsigned distance = ~0;

    for (unsigned i = 0; i < mColors.size(); ++i) {
        auto& colorAtIndex = mColors[i];
        int rOffset = (int)colorAtIndex.r - (int)color.r;
        int gOffset = (int)colorAtIndex.g - (int)color.g;
        int bOffset = (int)colorAtIndex.b - (int)color.b;

        unsigned currentDistance = rOffset * rOffset + gOffset * gOffset + bOffset * bOffset;

        if (currentDistance < distance) {
            distance = currentDistance;
            result = i;
        }
    }

    return PixelIu8(result);
}


std::unique_ptr<FileDefinition> PaletteDefinition::GenerateDefinition(const std::string& name, const std::string& location) const {
    std::unique_ptr<StructureDataChunk> dataChunk(new StructureDataChunk());

    for (unsigned chunkIndex = 0; chunkIndex < mData.size(); ++chunkIndex) {
        std::ostringstream stream;
        stream << "0x" << std::hex << std::setw(16) << std::setfill('0') << mData[chunkIndex];
        dataChunk->AddPrimitive(stream.str());
    }

    return std::unique_ptr<FileDefinition>(new DataFileDefinition("u64", name, true, location, std::move(dataChunk), this));
}

const std::string& PaletteDefinition::Name() const {
    return mName;
}


int gSizeInc[] = {3, 1, 0, 0};
int gSizeShift[] = {2, 1, 0, 0};

int PaletteDefinition::LoadBlockSize() const {
    return mColors.size() - 1;
}

int PaletteDefinition::NBytes() const {
    return mColors.size() * 2;
}

unsigned PaletteDefinition::ColorCount() const {
    return mColors.size();
}

TextureDefinition::TextureDefinition(const std::string& filename, G_IM_FMT fmt, G_IM_SIZ siz, TextureDefinitionEffect effects, std::shared_ptr<PaletteDefinition> palette) :
    TextureDefinition(
        new CImgu8(filename),
        getBaseName(replaceExtension(filename, "")) + "_" + gFormatShortName[(int)fmt] + "_" + gSizeName[(int)siz],
        fmt,
        siz,
        palette,
        effects
    ) {
}

TextureDefinition::~TextureDefinition() {
    delete mImg;
    mImg = NULL;
}

TextureDefinition::TextureDefinition(
    CImgu8* img,
    const std::string& name, 
    G_IM_FMT fmt, 
    G_IM_SIZ siz, 
    std::shared_ptr<PaletteDefinition> palette,
    TextureDefinitionEffect effects
): mImg(std::move(img)), mName(name), mFmt(fmt), mSiz(siz), mWidth(img->mImg.width()), mHeight(img->mImg.height()),
    mPalette(palette), mEffects(effects) {
    if (HasEffect(TextureDefinitionEffect::TwoToneGrayscale)) {
        applyTwoToneEffect(mImg->mImg, mTwoToneMax, mTwoToneMin);
    }

    if (HasEffect(TextureDefinitionEffect::NormalMap)) {
        calculateNormalMap(mImg->mImg);
    }

    if (HasEffect(TextureDefinitionEffect::Invert)) {
        invertImage(mImg->mImg);
    }

    if (HasEffect(TextureDefinitionEffect::SelectR) || 
        HasEffect(TextureDefinitionEffect::SelectG) || 
        HasEffect(TextureDefinitionEffect::SelectB)
    ) {
        selectChannel(mImg->mImg, mEffects);
    }

    int texelSwapMask = 0;
    if (HasEffect(TextureDefinitionEffect::PreSwapTexels)) {
        // Pre-swap texels so the RDP doesn't have to do it, which avoids
        // DXT counter imprecision artifacts for some texture sizes
        texelSwapMask = getTexelSwapMask(siz);
    }

    mWidth = mImg->mImg.width();
    mHeight = mImg->mImg.height();

    DataChunkStream dataStream;

    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            int readX = x;
            if (texelSwapMask && (y & 1)) {
                readX ^= texelSwapMask;
            }

            convertPixel(mImg->mImg, readX, y, dataStream, fmt, siz, palette);
        }
    }

    auto data = dataStream.GetData();
    mData.resize(data.size());

    std::copy(data.begin(), data.end(), mData.begin());

    if (palette) {
        mFmt = G_IM_FMT::G_IM_FMT_CI;
        mSiz = palette->ColorCount() <= 16 ? G_IM_SIZ::G_IM_SIZ_4b : G_IM_SIZ::G_IM_SIZ_8b;
    }
    
}
bool isGrayscale(cimg_library::CImg<unsigned char>& input, int x, int y) {
    switch (input.spectrum()) {
        case 1:
        case 2:
            return true;
        case 3:
        case 4:
            return input(x, y, 0, 0) == input(x, y, 0, 1) && input(x, y, 0, 1) == input(x, y, 0, 2);
    }

    return false;
}

int colorHash(cimg_library::CImg<unsigned char>& input, int x, int y) {
    switch (input.spectrum()) {
        case 1:
        case 2:
            return input(x, y, 0, 0);
        case 3:
        case 4:
            return (input(x, y, 0, 0) << 24) | (input(x, y, 0, 1) << 16) | (input(x, y, 0, 2) << 8);
    }

    return 0;
}

void TextureDefinition::DetermineIdealFormat(const std::string& filename, G_IM_FMT& fmt, G_IM_SIZ& siz) {
    cimg_library::CImg<unsigned char> imageData(filename.c_str());

    bool hasColor = false;
    bool hasFullTransparency = false;
    bool hasPartialTransparency = false;
    std::set<int> colorCount;

    for (int y = 0; y < imageData.height(); ++y) {
        for (int x = 0; x < imageData.width(); ++x) {
            colorCount.insert(colorHash(imageData, x, y));
            bool isPixelGrayscale = isGrayscale(imageData, x, y);
            hasColor = hasColor || !isPixelGrayscale;
            unsigned char alpha = imageData.spectrum() == 4 ? imageData(x, y, 0, 3) : 0xFF;

            hasPartialTransparency = hasPartialTransparency || (alpha != 0 && alpha != 0xFF);
            hasFullTransparency = hasFullTransparency || alpha == 0;
        }
    }

    if (hasColor) {
        if (hasPartialTransparency) {
            fmt = G_IM_FMT::G_IM_FMT_RGBA;
            siz = G_IM_SIZ::G_IM_SIZ_32b;
        } else {
            fmt = G_IM_FMT::G_IM_FMT_RGBA;
            siz = G_IM_SIZ::G_IM_SIZ_16b;
        }
    } else {
        if (hasPartialTransparency || hasFullTransparency) {
            fmt = G_IM_FMT::G_IM_FMT_IA;
            siz = G_IM_SIZ::G_IM_SIZ_16b;
        } else {
            fmt = G_IM_FMT::G_IM_FMT_I;
            siz = G_IM_SIZ::G_IM_SIZ_8b;
        }
    }
}

bool TextureDefinition::PspIs16Bit() const {
    // RGBA16 maps straight to the GU's 5551; anything else without a palette
    // widens to 8888.
    return mFmt == G_IM_FMT::G_IM_FMT_RGBA && mSiz == G_IM_SIZ::G_IM_SIZ_16b;
}

// Narrower mip levels are padded to this width rather than dropped.
#define PSP_MIN_BUFFER_WIDTH 8

// A texel packed as the GE reads it with no palette, 16 or 32 bits.
static uint32_t packPspPixel(const PixelRGBAu8& pixel, int bits) {
    if (bits == 16) {
        // GU 5551: alpha in bit 15, then blue, green, red.
        return (pixel.a >= 0x80 ? 0x8000u : 0u)
            | ((unsigned)(pixel.b >> 3) << 10)
            | ((unsigned)(pixel.g >> 3) << 5)
            | (unsigned)(pixel.r >> 3);
    }

    // GU 8888 is ABGR in memory, not RGBA.
    return ((unsigned)pixel.a << 24)
        | ((unsigned)pixel.b << 16)
        | ((unsigned)pixel.g << 8)
        | (unsigned)pixel.r;
}

// Narrowest buffer the GE fetches cleanly: 8 texels and at least 16 bytes a
// row. psp_model_render.c binds with the same rule.
static int pspMinBufferWidth(int bits) {
    return std::max(PSP_MIN_BUFFER_WIDTH, 128 / bits);
}

// The GE's swizzled layout: 16 byte by 8 row blocks, rows within a block
// consecutive, blocks in row order. Much faster to sample at an angle.
static bool pspCanSwizzle(int bufferWidth, int height, int bits) {
    int rowBytes = bufferWidth * bits / 8;
    return rowBytes % 16 == 0 && height % 8 == 0 && height >= 8;
}

// `texels` holds a palette index or a packed colour per texel, `bits` wide.
static std::unique_ptr<FileDefinition> writePspLevel(
    const std::vector<uint32_t>& texels,
    int width,
    int height,
    int bufferWidth,
    int bits,
    bool swizzle,
    const std::string& name,
    const std::string& location,
    const void* owner
) {
    std::unique_ptr<StructureDataChunk> dataChunk(new StructureDataChunk());

    const int rowBytes = bufferWidth * bits / 8;

    // Linear first. Padding repeats the last texel; T4 keeps the left texel of a
    // pair in the low nibble.
    std::vector<uint8_t> linear(rowBytes * height, 0);

    for (int y = 0; y < height; ++y) {
        uint8_t* row = &linear[y * rowBytes];

        for (int x = 0; x < bufferWidth; ++x) {
            int readX = x < width ? x : width - 1;
            uint32_t value = texels[readX + y * width];

            if (bits == 4) {
                row[x / 2] |= (value & 0xF) << ((x & 1) * 4);
            } else {
                for (int byte = 0; byte < bits / 8; ++byte) {
                    row[x * bits / 8 + byte] = (value >> (byte * 8)) & 0xFF;
                }
            }
        }
    }

    std::vector<uint8_t> ordered;

    if (swizzle) {
        ordered.reserve(linear.size());

        for (int blockY = 0; blockY < height / 8; ++blockY) {
            for (int blockX = 0; blockX < rowBytes / 16; ++blockX) {
                for (int row = 0; row < 8; ++row) {
                    auto start = linear.begin() + (blockY * 8 + row) * rowBytes + blockX * 16;
                    ordered.insert(ordered.end(), start, start + 16);
                }
            }
        }
    } else {
        ordered = linear;
    }

    // One buffer row per line: in texels for 16 and 32 bits, in bytes below.
    const int unitBytes = bits <= 8 ? 1 : bits / 8;

    for (int y = 0; y < height; ++y) {
        std::ostringstream stream;

        for (int unit = 0; unit < rowBytes / unitBytes; ++unit) {
            uint32_t value = 0;

            for (int byte = 0; byte < unitBytes; ++byte) {
                value |= (uint32_t)ordered[y * rowBytes + unit * unitBytes + byte] << (byte * 8);
            }

            if (unit != 0) {
                stream << ", ";
            }

            stream << "0x" << std::hex << std::setw(unitBytes * 2) << std::setfill('0') << value;
        }

        dataChunk->AddPrimitive(stream.str());
    }

    // The GE fetches texels from a 16 byte boundary.
    const char* type = unitBytes == 1 ? "unsigned char __attribute__((aligned(16)))"
        : unitBytes == 2 ? "unsigned short __attribute__((aligned(16)))"
        : "unsigned int __attribute__((aligned(16)))";

    return std::unique_ptr<FileDefinition>(new DataFileDefinition(
        type, name, true, location, std::move(dataChunk), owner));
}

// Box filter.
static std::vector<PixelRGBAu8> downsamplePsp(
    const std::vector<PixelRGBAu8>& pixels, int width, int height
) {
    int halfWidth = width / 2;
    int halfHeight = height / 2;

    std::vector<PixelRGBAu8> result(halfWidth * halfHeight);

    for (int y = 0; y < halfHeight; ++y) {
        for (int x = 0; x < halfWidth; ++x) {
            unsigned r = 0, g = 0, b = 0, a = 0;

            for (int dy = 0; dy < 2; ++dy) {
                for (int dx = 0; dx < 2; ++dx) {
                    const PixelRGBAu8& source = pixels[(x * 2 + dx) + (y * 2 + dy) * width];
                    r += source.r;
                    g += source.g;
                    b += source.b;
                    a += source.a;
                }
            }

            result[x + y * halfWidth] = PixelRGBAu8(r / 4, g / 4, b / 4, a / 4);
        }
    }

    return result;
}

// The GE takes power of two sizes, so others are padded up.
int TextureDefinition::PspPaddedSize(int value) {
    int result = 1;

    while (result < value) {
        result *= 2;
    }

    return result;
}

std::vector<std::unique_ptr<FileDefinition>> TextureDefinition::GeneratePspDefinitions(const std::string& baseName, const std::string& location, bool mirrorS, bool mirrorT, bool alphaOnly, bool invert, bool* swizzled, std::string* format, std::unique_ptr<FileDefinition>* clut) const {
    // readRGBAPixel() takes a mutable reference but only reads.
    cimg_library::CImg<unsigned char>& image = const_cast<CImgu8*>(mImg)->mImg;

    const bool is16Bit = PspIs16Bit();

    const int contentWidth = mirrorS ? mWidth * 2 : mWidth;
    const int contentHeight = mirrorT ? mHeight * 2 : mHeight;

    // Pad rather than resample, so texel coordinates used by the 2D code stay
    // valid. Content at the top left, padding repeats the edge.
    int width = PspPaddedSize(contentWidth);
    int height = PspPaddedSize(contentHeight);

    std::vector<PixelRGBAu8> pixels(width * height);

    for (int y = 0; y < height; ++y) {
        const int clampY = y < contentHeight ? y : contentHeight - 1;
        const int readY = (clampY < mHeight) ? clampY : (mHeight * 2 - 1 - clampY);

        for (int x = 0; x < width; ++x) {
            const int clampX = x < contentWidth ? x : contentWidth - 1;
            const int readX = (clampX < mWidth) ? clampX : (mWidth * 2 - 1 - clampX);

            PixelRGBAu8 pixel = readRGBAPixel(image, readX, readY);

            // An I texel's intensity is also its alpha on the RDP (the font's combiner
            // relies on it); readRGBAPixel() returns it opaque.
            if (mFmt == G_IM_FMT::G_IM_FMT_I) {
                pixel.a = pixel.r;
            }

            // Two tone textures: the N64 recolours the grey in the combiner with
            // lerp(ENVIRONMENT, PRIMITIVE, TEXEL0). The GE cannot, so bake it here.
            if (HasEffect(TextureDefinitionEffect::TwoToneGrayscale)) {
                int i = pixel.r;
                pixel.r = mTwoToneMin.r + (mTwoToneMax.r - mTwoToneMin.r) * i / 255;
                pixel.g = mTwoToneMin.g + (mTwoToneMax.g - mTwoToneMin.g) * i / 255;
                pixel.b = mTwoToneMin.b + (mTwoToneMax.b - mTwoToneMin.b) * i / 255;
            }

            // Alpha only: white, so MODULATE keeps the vertex colour. See
            // pspTextureIsAlphaOnly().
            if (alphaOnly) {
                pixel.r = pixel.g = pixel.b = 0xFF;
            }

            // Inverted for GU_TFX_BLEND. See pspAnalyzeCombine().
            if (invert) {
                pixel.r = 0xFF - pixel.r;
                pixel.g = 0xFF - pixel.g;
                pixel.b = 0xFF - pixel.b;
            }

            pixels[x + y * width] = pixel;
        }
    }

    // The whole chain first: the palette is chosen from all of it. The GU
    // allows levels 0 to 7.
    struct PspLevel {
        std::vector<PixelRGBAu8> pixels;
        int width;
        int height;
    };

    std::vector<PspLevel> chain;

    for (int level = 0; level < 8; ++level) {
        chain.push_back({pixels, width, height});

        if (width < 2 || height < 2) {
            break;
        }

        pixels = downsamplePsp(pixels, width, height);
        width /= 2;
        height /= 2;
    }

    // Up to 256 colours: palette indices, T4 for 16 or fewer, T8 above. Level 0
    // keeps its colours exactly; mip levels use the nearest palette entry once
    // the palette is full. RGBA16 keeps its one alpha bit.
    std::map<uint32_t, uint32_t> indexOf;
    std::vector<uint32_t> palette;

    auto paletteColour = [is16Bit](const PixelRGBAu8& pixel) {
        PixelRGBAu8 colour = pixel;

        if (is16Bit) {
            colour.a = colour.a >= 0x80 ? 0xFF : 0;
        }

        return packPspPixel(colour, 32);
    };

    auto addColours = [&](const std::vector<PixelRGBAu8>& levelPixels) {
        for (const PixelRGBAu8& pixel : levelPixels) {
            if (indexOf.emplace(paletteColour(pixel), (uint32_t)palette.size()).second) {
                palette.push_back(paletteColour(pixel));
            }
        }
    };

    addColours(chain[0].pixels);

    int bits = is16Bit ? 16 : 32;

    if (palette.size() <= 256) {
        const size_t ownColours = palette.size();
        const size_t paletteSize = ownColours <= 16 ? 16 : 256;

        for (size_t level = 1; level < chain.size(); ++level) {
            addColours(chain[level].pixels);
        }

        if (palette.size() > paletteSize) {
            palette.resize(ownColours);

            for (auto it = indexOf.begin(); it != indexOf.end();) {
                it = it->second >= ownColours ? indexOf.erase(it) : std::next(it);
            }
        }

        bits = paletteSize == 16 ? 4 : 8;
        palette.resize(paletteSize, 0);
    } else {
        palette.clear();
    }

    auto texelOf = [&](const PixelRGBAu8& pixel) -> uint32_t {
        if (palette.empty()) {
            return packPspPixel(pixel, bits);
        }

        uint32_t colour = paletteColour(pixel);
        auto found = indexOf.find(colour);

        if (found != indexOf.end()) {
            return found->second;
        }

        uint32_t nearest = 0;
        int nearestDistance = INT32_MAX;

        for (const auto& entry : indexOf) {
            int distance = 0;

            for (int shift = 0; shift < 32; shift += 8) {
                int difference = (int)((colour >> shift) & 0xFF) - (int)((entry.first >> shift) & 0xFF);
                distance += difference * difference;
            }

            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearest = entry.second;
            }
        }

        return nearest;
    };

    if (format) {
        *format = bits == 4 ? "GU_PSM_T4" : bits == 8 ? "GU_PSM_T8" : bits == 16 ? "GU_PSM_5551" : "GU_PSM_8888";
    }

    if (clut && !palette.empty()) {
        std::unique_ptr<StructureDataChunk> clutChunk(new StructureDataChunk());

        for (size_t start = 0; start < palette.size(); start += 8) {
            std::ostringstream stream;

            for (size_t entry = start; entry < start + 8; ++entry) {
                if (entry != start) {
                    stream << ", ";
                }

                stream << "0x" << std::hex << std::setw(8) << std::setfill('0') << palette[entry];
            }

            clutChunk->AddPrimitive(stream.str());
        }

        // Loaded by the GE in blocks of eight entries from a 16 byte boundary.
        *clut = std::unique_ptr<FileDefinition>(new DataFileDefinition(
            "unsigned int __attribute__((aligned(16)))", baseName + "_clut", true, location, std::move(clutChunk), this));
    }

    std::vector<std::unique_ptr<FileDefinition>> result;

    // Swizzled when the top level can be; the chain stops at the first level
    // too small for the blocks.
    bool swizzle = pspCanSwizzle(std::max(pspMinBufferWidth(bits), chain[0].width), chain[0].height, bits);

    if (swizzled) {
        *swizzled = swizzle;
    }

    for (size_t level = 0; level < chain.size(); ++level) {
        const PspLevel& source = chain[level];
        std::string name = level == 0 ? baseName : baseName + "_mip" + std::to_string(level);
        int bufferWidth = std::max(pspMinBufferWidth(bits), source.width);

        if (swizzle && !pspCanSwizzle(bufferWidth, source.height, bits)) {
            break;
        }

        std::vector<uint32_t> texels(source.pixels.size());

        for (size_t i = 0; i < texels.size(); ++i) {
            texels[i] = texelOf(source.pixels[i]);
        }

        result.push_back(writePspLevel(texels, source.width, source.height, bufferWidth, bits, swizzle, name, location, this));
    }

    return result;
}

std::unique_ptr<FileDefinition> TextureDefinition::GenerateDefinition(const std::string& name, const std::string& location) const {
    std::unique_ptr<StructureDataChunk> dataChunk(new StructureDataChunk());

    int line;
    int index = 0;

    GetLine(line);

    for (int y = 0; y < mHeight; ++y) {
        std::ostringstream stream;

        for (int lineIndex = 0; lineIndex < line; ++lineIndex) {
            uint64_t data = mData[index];

            if (lineIndex != 0) {
                stream << ", ";
            }

            stream << "0x" << std::hex << std::setw(16) << std::setfill('0') << data;

            ++index;
        }

        dataChunk->AddPrimitive(stream.str());
    }

    return std::unique_ptr<FileDefinition>(new DataFileDefinition("u64", name, true, location, std::move(dataChunk), this));
}

int TextureDefinition::Width() const {
    return mWidth;
}

int TextureDefinition::Height() const {
    return mHeight;
}

G_IM_FMT TextureDefinition::Format() const {
    return mFmt;
}

G_IM_SIZ TextureDefinition::Size() const {
    return mSiz;
}

int TextureDefinition::LoadBlockSize() const {
    return ((Height() * Width() + gSizeInc[(int)mSiz]) >> gSizeShift[(int)mSiz]) - 1;
}

int TextureDefinition::DXT() const {
    int lineSize;

    if (mSiz == G_IM_SIZ::G_IM_SIZ_4b) {
        lineSize = Width() / 16;
    } else {
        GetLine(lineSize);
    }

    if (!lineSize) {
        lineSize = 1;
    }

    return ((1 << 11) + lineSize - 1) / lineSize;
}

int TextureDefinition::NBytes() const {
    int line;
    GetLine(line);
    return mHeight * line * 8;
}

bool TextureDefinition::GetLine(int& line) const {
    int bitLine = bitSizeforSiz(mSiz) * mWidth;
    line = bitLine / 64;
    return bitLine % 64 == 0;
}

bool TextureDefinition::GetLineForTile(int& line) const {
    int bitLine = lineSizeForSize(mSiz) * mWidth;
    line = bitLine / 64;
    return bitLine % 64 == 0;
}

const std::vector<unsigned long long>& TextureDefinition::GetData() const {
    return mData;
}

const std::string& TextureDefinition::Name() const {
    return mName;
}

bool TextureDefinition::HasEffect(TextureDefinitionEffect effect) const {
    return (int)mEffects & (int)effect;
}

PixelRGBAu8 TextureDefinition::GetTwoToneMin() const {
    return mTwoToneMin;
}

PixelRGBAu8 TextureDefinition::GetTwoToneMax() const {
    return mTwoToneMax;
}

std::shared_ptr<PaletteDefinition> TextureDefinition::GetPalette() const {
    return mPalette;
}

std::shared_ptr<TextureDefinition> TextureDefinition::Crop(int x, int y, int w, int h) const {
    return std::shared_ptr<TextureDefinition>(new TextureDefinition(
        new CImgu8(mImg->mImg.get_crop(x, y, x + w - 1, y + h - 1)),
        mName,
        mFmt,
        mSiz,
        mPalette,
        mEffects
    ));
}

std::shared_ptr<TextureDefinition> TextureDefinition::Resize(int w, int h) const {
    return std::shared_ptr<TextureDefinition>(new TextureDefinition(
        new CImgu8(mImg->mImg.get_resize(w, h, -100, -100, 5)),
        mName,
        mFmt,
        mSiz,
        mPalette,
        mEffects
    ));
}