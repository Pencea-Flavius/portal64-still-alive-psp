const fs = require("fs");
const util = require("./model_list_utils");

function generateLevelListEntry(outputPath, levelHeader) {
    const levelName = util.generateModelName(levelHeader);
    return `    {
        &${util.generateRelativeModelName(outputPath, levelHeader, "_level")},
        _${levelName}_geoSegmentRomStart,
        _${levelName}_geoSegmentRomEnd,
        _${levelName}_geoSegmentStart,
    },`;
}

// There are no segments on the PSP: every level is linked and resident, so an
// entry is the level and nothing else.
function generatePspLevelListEntry(outputPath, levelHeader) {
    return `    {
        &${util.generateRelativeModelName(outputPath, levelHeader, "_level")},
    },`;
}

function generateHeader(outputPath, config) {
    const { modelHeaders, modelGroup, externGenerator = util.generateExterns } = config;

    const blocks = [
        `#include "levels/level_metadata.h"`,
        util.generateIncludes(outputPath, modelHeaders),
        util.generateCount(modelGroup, modelHeaders),
        externGenerator(modelHeaders),
        util.generateModelList(outputPath, config),
    ];

    return util.wrapWithIncludeGuard(outputPath, blocks.filter(block => block).join("\n\n"));
}

function generateNoExterns() {
    return "";
}

const args = process.argv.slice(2);
const targetPsp = args.includes("--psp");
const [outputHeaderFile, ...modelHeaders] = args.filter(arg => arg !== "--psp");

const config = {
    modelHeaders,
    modelGroup: "level",
    modelType: "LevelMetadata",
    listEntryGenerator: targetPsp ? generatePspLevelListEntry : generateLevelListEntry,
    ...(targetPsp && { externGenerator: generateNoExterns }),
};

fs.writeFileSync(
    outputHeaderFile,
    generateHeader(outputHeaderFile, config)
);
