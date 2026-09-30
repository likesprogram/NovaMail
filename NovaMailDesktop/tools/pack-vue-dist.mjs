import { createHash } from 'node:crypto';
import {
    copyFile,
    mkdir,
    readdir,
    readFile,
    rm,
    stat,
    writeFile,
} from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const START_ID = 2000;
const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const workspaceDir = path.resolve(scriptDir, '..');

function usage() {
    console.error(
        'Usage: node tools/pack-vue-dist.mjs <dist-dir> <resource-output-dir>',
    );
}

function toRcPath(projectDir, filePath) {
    return path.relative(projectDir, filePath).replaceAll(path.sep, '\\\\');
}

async function exists(filePath) {
    try {
        await stat(filePath);
        return true;
    } catch {
        return false;
    }
}

async function walkFiles(rootDir, currentDir = rootDir) {
    const entries = await readdir(currentDir, { withFileTypes: true });
    const files = [];

    for (const entry of entries) {
        const nextPath = path.join(currentDir, entry.name);
        if (entry.isDirectory()) {
            files.push(...(await walkFiles(rootDir, nextPath)));
            continue;
        }

        if (entry.isFile()) {
            files.push(path.relative(rootDir, nextPath).replaceAll(path.sep, '/'));
        }
    }

    return files.sort((a, b) => a.localeCompare(b));
}

function assertSafeTarget(sourceDir, targetDir) {
    const relativeTarget = path.relative(workspaceDir, targetDir);
    if (relativeTarget.startsWith('..') || path.isAbsolute(relativeTarget)) {
        throw new Error(`Refusing to write outside workspace: ${targetDir}`);
    }

    if (!relativeTarget.replaceAll(path.sep, '/').includes('/res/vue-dist')) {
        throw new Error(
            'Refusing to overwrite an unexpected folder. Use a target like NovaMailDesktop/res/vue-dist.',
        );
    }

    if (path.resolve(sourceDir) === path.resolve(targetDir)) {
        throw new Error('Source dist directory and resource output directory match.');
    }
}

async function main() {
    const [, , sourceArg, targetArg] = process.argv;
    if (!sourceArg || !targetArg) {
        usage();
        process.exitCode = 1;
        return;
    }

    const sourceDir = path.resolve(process.cwd(), sourceArg);
    const targetDir = path.resolve(process.cwd(), targetArg);
    const resourceDir = path.dirname(targetDir);
    const projectDir = path.dirname(resourceDir);
    const generatedRcPath = path.join(resourceDir, 'vue-dist-generated.rc');
    const generatedHeaderPath = path.join(resourceDir, 'vue-dist-resources.h');
    const generatedMapPath = path.join(resourceDir, 'vue-dist-resource-map.h');

    assertSafeTarget(sourceDir, targetDir);

    const indexPath = path.join(sourceDir, 'index.html');
    if (!(await exists(indexPath))) {
        throw new Error(`Cannot find Vue entry file: ${indexPath}`);
    }

    const sourceFiles = await walkFiles(sourceDir);
    await rm(targetDir, { recursive: true, force: true });
    await mkdir(targetDir, { recursive: true });

    const manifest = {
        entry: 'index.html',
        files: [],
    };

    for (const [index, relativePath] of sourceFiles.entries()) {
        const sourceFile = path.join(sourceDir, ...relativePath.split('/'));
        const targetFile = path.join(targetDir, ...relativePath.split('/'));
        const content = await readFile(sourceFile);

        await mkdir(path.dirname(targetFile), { recursive: true });
        await copyFile(sourceFile, targetFile);

        manifest.files.push({
            id: START_ID + 1 + index,
            name: `IDR_VUE_DIST_FILE_${index}`,
            path: relativePath,
            size: content.byteLength,
            sha256: createHash('sha256').update(content).digest('hex'),
        });
    }

    const manifestPath = path.join(targetDir, 'manifest.json');
    await writeFile(manifestPath, `${JSON.stringify(manifest, null, 2)}\n`, 'utf8');

    const headerLines = [
        '#pragma once',
        '',
        `#define IDR_VUE_DIST_MANIFEST ${START_ID}`,
        ...manifest.files.map((file) => `#define ${file.name} ${file.id}`),
        '',
    ];

    const rcLines = [
        '#include "vue-dist-resources.h"',
        '',
        `IDR_VUE_DIST_MANIFEST RCDATA "${toRcPath(projectDir, manifestPath)}"`,
        ...manifest.files.map((file) => {
            const filePath = path.join(targetDir, ...file.path.split('/'));
            return `${file.name} RCDATA "${toRcPath(projectDir, filePath)}"`;
        }),
        '',
    ];

    const mapLines = [
        '#pragma once',
        '',
        '#include "vue-dist-resources.h"',
        '',
        'namespace NovaMailVueDist {',
        '',
        'struct ResourceEntry {',
        '  int id;',
        '  const wchar_t* path;',
        '};',
        '',
        'inline constexpr const wchar_t* kEntryFile = L"index.html";',
        '',
        'inline constexpr ResourceEntry kResources[] = {',
        ...manifest.files.map((file) => {
          const escapedPath = file.path.replaceAll('/', '\\\\').replaceAll('"', '\\"');
          return `    {${file.name}, L"${escapedPath}"},`;
        }),
        '};',
        '',
        '}  // namespace NovaMailVueDist',
        '',
    ];

    await writeFile(generatedHeaderPath, headerLines.join('\n'), 'utf8');
    await writeFile(generatedRcPath, rcLines.join('\n'), 'utf8');
    await writeFile(generatedMapPath, mapLines.join('\n'), 'utf8');

    console.log(`Packed ${manifest.files.length} Vue dist files.`);
    console.log(`Resource manifest: ${manifestPath}`);
    console.log(`Generated RC: ${generatedRcPath}`);
    console.log(`Generated C++ map: ${generatedMapPath}`);
}

main().catch((error) => {
    console.error(error instanceof Error ? error.message : error);
    process.exitCode = 1;
});
