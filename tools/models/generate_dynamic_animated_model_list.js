const fs = require("fs");
const path = require("path");
const util = require("./model_list_utils");

function generateModelListEntry(outputPath, modelHeader) {
    const modelName = util.generateModelName(modelHeader);
    return `    {
        _${modelName}_geoSegmentRomStart,
        _${modelName}_geoSegmentRomEnd,
        _${modelName}_geoSegmentStart,
        &${util.generateRelativeModelName(outputPath, modelHeader, "_armature")},
        ${util.generateRelativeModelName(outputPath, modelHeader, "_clips")},
        ${util.generateRelativeModelName(outputPath, modelHeader, "_clip_count").toUpperCase()},
        "${modelName}",
    },`;
}

// There are no segments on the PSP: the whole model set is linked, so an entry
// is the armature and its clips and nothing is copied anywhere.
function generatePspModelListEntry(outputPath, modelHeader) {
    const modelName = util.generateModelName(modelHeader);
    return `    {
        &${util.generateRelativeModelName(outputPath, modelHeader, "_armature")},
        ${util.generateRelativeModelName(outputPath, modelHeader, "_clips")},
        ${util.generateRelativeModelName(outputPath, modelHeader, "_clip_count").toUpperCase()},
        "${modelName}",
    },`;
}

function generateNoExterns() {
    return "";
}

const args = process.argv.slice(2);
const targetPsp = args.includes("--psp");
const [outputHeaderFile, ...modelHeaders] = args.filter(arg => arg !== "--psp");

const { dir: outputDir, name: outputName } = path.parse(outputHeaderFile);
const outputSourceFile = `${outputDir}/${outputName}.c`

const config = {
    modelHeaders,
    modelGroup: "dynamic_animated_model",
    modelType: "DynamicAnimatedAssetModel",
    listEntryGenerator: targetPsp ? generatePspModelListEntry : generateModelListEntry,
    ...(targetPsp && { externGenerator: generateNoExterns }),
};

fs.writeFileSync(
    outputHeaderFile,
    util.generateHeader(outputHeaderFile, config)
);
fs.writeFileSync(
    outputSourceFile,
    util.generateData(outputSourceFile, config)
);
