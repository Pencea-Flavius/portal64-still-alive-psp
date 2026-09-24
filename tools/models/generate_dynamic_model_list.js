const fs = require("fs");
const path = require("path");
const util = require("./model_list_utils");

function generateModelListEntry(outputPath, modelHeader) {
    const modelName = util.generateModelName(modelHeader);
    return `    {
        _${modelName}_geoSegmentRomStart,
        _${modelName}_geoSegmentRomEnd,
        _${modelName}_geoSegmentStart,
        ${util.generateRelativeModelName(outputPath, modelHeader, "_model_gfx")},
        "${modelName}",
    },`;
}

// There are no segments on the PSP: the whole model set is linked, so an entry
// is the model and its name and nothing is copied anywhere. The model is a
// struct PspModel rather than a Gfx array, so the handle is its address.
function generatePspModelListEntry(outputPath, modelHeader) {
    const modelName = util.generateModelName(modelHeader);
    return `    {
        &${util.generateRelativeModelName(outputPath, modelHeader, "_model")},
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
    modelGroup: "dynamic_model",
    modelType: "DynamicAssetModel",
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
