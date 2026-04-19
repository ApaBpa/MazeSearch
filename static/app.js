
const ZOOM_MIN = 1;
const ZOOM_MAX = 500;
const ZOOM_STEP = 1;
const fmtMs = (v) => Number.isFinite(v) ? v.toFixed(2) : '-';

let mazeDataArr = [];
let pathDataArr = [];
let currentMazeIdx = 0;
let runCount = 0;
let results = [];
let pendingResultIndexes = [];
let graphMode = false;
let zoomLevel = 1;
let panX = 0;
let panY = 0;
let isDragging = false;
let dragStartX = 0;
let dragStartY = 0;
let dragOriginX = 0;
let dragOriginY = 0;

// Generate maze
async function onBtnGenerateClick() {
    const width = Number(document.getElementById('widthInput').value);
    const height = Number(document.getElementById('heightInput').value);
    const generator = document.getElementById('generatorSelect').value;

    if (!Number.isFinite(width) || !Number.isFinite(height) || width < 1 || height < 1) {
        document.getElementById('genInfo').textContent = 'Width and height must be positive numbers.';
        return;
    }

    const mazesToGenerate = Number(document.getElementById('numMazes').value);

    try {
        // HTTP POST to trigger generation, then GET to fetch data
        const res = await fetch('/generate?width=' + width + '&height=' + height + '&numMazes=' + mazesToGenerate, { method: 'POST' });
        if (!res.ok) {
            throw new Error('Generate request failed with status ' + res.status);
        }

        const data = await res.json();
        mazeDataArr = data.mazes || [data];
        pathDataArr = new Array(mazeDataArr.length).fill(null);

        pendingResultIndexes = [];
        for (let i = 0; i < mazeDataArr.length; i++) {
            runCount++;
            results.push({
                id: runCount,
                gen: generator,
                algo: 'Pending',
                w: mazeDataArr[i].width,
                h: mazeDataArr[i].height,
                cells: mazeDataArr[i].width * mazeDataArr[i].height,
                genMs: Number(mazeDataArr[i].generationTime) ?? data.generationTime / mazeDataArr.length,
                solveMs: null,
                pathLen: null,
                efficiency: null
            });
            pendingResultIndexes.push(results.length - 1);
        }

        document.getElementById('genInfo').textContent = `Generated ${width}x${height} maze(s) in ${fmtMs(Number(data.generationTime))} ms.`;
        document.getElementById('btnSolve').disabled = false;
        RenderResults();
        RenderMazeScroll();
    } catch (err) {
        console.error('Generate failed', err);
        document.getElementById('genInfo').textContent = 'Failed to generate maze.';
    }
}

document.getElementById('btnGenerate').onclick = onBtnGenerateClick;

// Solve maze
document.getElementById('btnSolve').onclick = async () => {
    const algorithm = document.getElementById('algorithmSelect').value;
    const mode = document.getElementById('modeSelect').value;
    const algorithmName = document.getElementById('algorithmSelect').selectedOptions[0].text
    const generatorName = document.getElementById('generatorSelect').selectedOptions[0].text;
    // HTTP POST to trigger solving, then GET to fetch solution
    const res = await fetch('/solve?algorithm=' + algorithm + '&mode=' + mode, { method: 'POST' });
    const data = await res.json();
    pathDataArr = data.paths || [];
    const firstPathLength = (pathDataArr[0] || []).length;
    if (pendingResultIndexes.length === mazeDataArr.length) {
        for (let i = 0; i < mazeDataArr.length; i++) {
            const resultIndex = pendingResultIndexes[i];
            const pathLength = (pathDataArr[i] || []).length;
            results[resultIndex].gen = generatorName;
            results[resultIndex].algo = algorithmName;
            results[resultIndex].solveMs = data.solvingTime;
            results[resultIndex].pathLen = pathLength;
            results[resultIndex].efficiency = ((pathLength / (mazeDataArr[i].width * mazeDataArr[i].height)) * 100).toFixed(2);
        }
        pendingResultIndexes = [];
    } else {
        for (let i = 0; i < mazeDataArr.length; i++) {
            const pathLength = (pathDataArr[i] || []).length;
            runCount++;
            results.push({
                id: runCount, gen: generatorName, algo: algorithmName,
                w: mazeDataArr[i].width, h: mazeDataArr[i].height,
                cells: mazeDataArr[i].width * mazeDataArr[i].height,
                genMs: mazeDataArr[i].generationTime,
                solveMs: data.solvingTime,
                pathLen: pathLength,
                efficiency: ((pathLength / (mazeDataArr[i].width * mazeDataArr[i].height)) * 100).toFixed(2)
            });
        }
    }
    RenderResults();
    RenderMazeScroll();
}
// Clear results
document.getElementById('btnClear').onclick = () => {
    results = [];
    runCount = 0;
    pendingResultIndexes = [];
    RenderResults();
}
function refreshToggleLabel() {
    const btn = document.getElementById('toggleViewBtn');
    if (!btn) return;
    btn.textContent = graphMode ? 'Maze Representation' : 'Graph Representation';
}
const toggleViewBtn = document.getElementById('toggleViewBtn');
if (toggleViewBtn) {
    toggleViewBtn.addEventListener('click', () => {
        graphMode = !graphMode;
        refreshToggleLabel();
        RenderMazeScroll();
    });
}
refreshToggleLabel();
function RenderMazeScroll() {
    const canvas = document.getElementById('mazeCanvas');
    if (!canvas) return;
    const mazeData = mazeDataArr[currentMazeIdx];
    const pathData = pathDataArr[currentMazeIdx] || [];
    if (!mazeData) return;
    if (graphMode) DrawGraph(canvas, mazeData, pathData);
    else DrawMaze(canvas, mazeData, pathData);
    const info = document.getElementById('mazeIndexInfo');
    if (info) info.textContent = `Maze ${currentMazeIdx + 1} of ${mazeDataArr.length}`;
}
function clamp(v, min, max) { return Math.min(Math.max(v, min), max); }
function setZoom(level) {
    zoomLevel = clamp(level, ZOOM_MIN, ZOOM_MAX);
    const resetBtn = document.getElementById('btn-reset-zoom');
    if (resetBtn) resetBtn.textContent = `${Math.round(zoomLevel * 100)}%`;
    RenderMazeScroll();
}
function applyZoom(ctx, canvas) {
    ctx.setTransform(1, 0, 0, 1, 0, 0); // Reset transform
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    ctx.translate(centerX + panX, centerY + panY);
    ctx.scale(zoomLevel, zoomLevel);
    ctx.translate(-centerX, -centerY);
}
function resetView() {
    panX = 0;
    panY = 0;
}
document.getElementById('btn-zoom-in').onclick = () => setZoom(zoomLevel + ZOOM_STEP);
document.getElementById('btn-zoom-out').onclick = () => setZoom(zoomLevel - ZOOM_STEP);

const btnResetZoom = document.getElementById('btn-reset-zoom');
btnResetZoom.addEventListener('click', () => {
    resetView();
    setZoom(1);
});

function updateCanvasCursor() {
    const canvas = document.getElementById('mazeCanvas');
    if (!canvas) return;
    canvas.style.cursor = isDragging ? 'grabbing' : 'grab';
}
function setupCanvasDrag() {
    const canvas = document.getElementById('mazeCanvas');
    if (!canvas) return;
    canvas.style.touchAction = 'none';
    updateCanvasCursor();
    canvas.addEventListener('pointerdown', (e) => {
        if (e.button !== 0) return;
        isDragging = true;
        dragStartX = e.clientX;
        dragStartY = e.clientY;
        dragOriginX = panX;
        dragOriginY = panY;
        canvas.setPointerCapture(e.pointerId);
        updateCanvasCursor();
    });
    canvas.addEventListener('pointermove', (e) => {
        if (!isDragging) return;
        panX = dragOriginX + (e.clientX - dragStartX);
        panY = dragOriginY + (e.clientY - dragStartY);
        RenderMazeScroll();
    });
    const stopDragging = (e) => {
        if (!isDragging) return;
        isDragging = false;
        if (canvas.hasPointerCapture(e.pointerId)) {
            canvas.releasePointerCapture(e.pointerId);
        }
        updateCanvasCursor();
    };
    canvas.addEventListener('pointerup', stopDragging);
    canvas.addEventListener('pointercancel', stopDragging);
}
setupCanvasDrag();
// Results rendering
function RenderResults() {
    const body = document.getElementById('resultsBody');
    if (!results.length) {
        body.innerHTML = '<tr><td colspan="9" style="color:#555">No results yet.</td></tr>';
        return;
    }
    const solvedResults = results.filter(r => typeof r.solveMs === 'number');
    const minSolve = solvedResults.length ? Math.min(...solvedResults.map(r => r.solveMs)) : null;
    body.innerHTML = results.slice().reverse().map(r => `
<tr class="${minSolve !== null && r.solveMs === minSolve ? 'fastest' : ''}">
    <td>${r.id}</td>
    <td>${r.gen}</td>
    <td>${r.algo}</td>
    <td>${r.w}x${r.h}</td>
    <td>${r.cells}</td>
    <td>${fmtMs(Number(r.genMs))}</td>
    <td>${fmtMs(Number(r.solveMs))}</td>
    <td>${r.pathLen === null ? '-' : r.pathLen}</td>
    <td>${r.efficiency === null ? '-' : r.efficiency + '%'}</td>
</tr>
`).join('');
}
// Draw maze
function DrawMaze(canvas, mazeData, pathData) {
    if (!mazeData) return;
    const ctx = canvas.getContext('2d');
    applyZoom(ctx, canvas);
    const { width, height, passages, start, finish } = mazeData;
    const cell_size = Math.min(canvas.width / width, canvas.height / height);
    const maze_w = cell_size * width;
    const maze_h = cell_size * height;
    const offset_x = (canvas.width - maze_w) / 2;
    const offset_y = (canvas.height - maze_h) / 2;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = '#1e1e1e';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.lineCap = 'round';
    const passageSet = new Set();
    passages.forEach(p => {
        passageSet.add(p.c1.x + ',' + p.c1.y + '-' + p.c2.x + ',' + p.c2.y);
        passageSet.add(p.c2.x + ',' + p.c2.y + '-' + p.c1.x + ',' + p.c1.y);
    });
    const isPassage = (x1, y1, x2, y2) => passageSet.has(x1 + ',' + y1 + '-' + x2 + ',' + y2);
    // Solution path
    if (pathData && pathData.length > 0) {
        ctx.fillStyle = '#26a4a6';
        pathData.forEach(c => ctx.fillRect(c.x * cell_size + offset_x, c.y * cell_size + offset_y, cell_size, cell_size));
    }
    // Start & Finish
    if (start) {
        ctx.fillStyle = '#4ec94e';
        ctx.fillRect(start.x * cell_size + offset_x, start.y * cell_size + offset_y, cell_size, cell_size);
    }
    if (finish) {
        ctx.fillStyle = '#e05252';
        ctx.fillRect(finish.x * cell_size + offset_x, finish.y * cell_size + offset_y, cell_size, cell_size);
    }
    // Walls
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 4;
    ctx.beginPath();
    ctx.rect(offset_x + 0.5, offset_y + 0.5, maze_w - 1, maze_h - 1);
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const cell_x = offset_x + x * cell_size;
            const cell_y = offset_y + y * cell_size;
            // Right wall
            if (x < width - 1 && !isPassage(x, y, x + 1, y)) {
                ctx.moveTo(cell_x + cell_size, cell_y);
                ctx.lineTo(cell_x + cell_size, cell_y + cell_size);
            }
            // Bottom wall
            if (y < height - 1 && !isPassage(x, y, x, y + 1)) {
                ctx.moveTo(cell_x, cell_y + cell_size);
                ctx.lineTo(cell_x + cell_size, cell_y + cell_size);
            }
        }
    }
    ctx.stroke();
    ctx.setTransform(1, 0, 0, 1, 0, 0);
}
// Draw maze in the style of a graph
function DrawGraph(canvas, mazeData, pathData) {
    if (!mazeData) return;
    applyZoom(canvas.getContext('2d'), canvas);
    const ctx = canvas.getContext('2d');
    const { width, height, passages, start, finish } = mazeData;
    const cell_size = Math.min(canvas.width / width, canvas.height / height);
    const maze_w = cell_size * width;
    const maze_h = cell_size * height;
    const offset_x = (canvas.width - maze_w) / 2;
    const offset_y = (canvas.height - maze_h) / 2;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = '#1e1e1e';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    const edges = new Set();
    function edgeKey(x1, y1, x2, y2) { return `${x1},${y1}|${x2},${y2}`; }
    (mazeData.passages || []).forEach(p => {
        edges.add(edgeKey(p.c1.x, p.c1.y, p.c2.x, p.c2.y));
        edges.add(edgeKey(p.c2.x, p.c2.y, p.c1.x, p.c1.y));
    });
    // Draw solution path
    if (Array.isArray(pathData) && pathData.length > 1) {
        ctx.beginPath();
        ctx.strokeStyle = '#26a4a6';
        ctx.lineWidth = 6;
        ctx.lineCap = 'round';
        pathData.forEach((p, i) => {
            const px = offset_x + p.x * cell_size + cell_size / 2;
            const py = offset_y + p.y * cell_size + cell_size / 2;
            i === 0 ? ctx.moveTo(px, py) : ctx.lineTo(px, py);
        });
        ctx.stroke();
    }
    // Draw edges between nodes
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 2;
    ctx.beginPath();
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const cx = offset_x + x * cell_size + cell_size / 2;
            const cy = offset_y + y * cell_size + cell_size / 2;
            [{ dx: 1, dy: 0 }, { dx: 0, dy: 1 }].forEach(d => { // Iterate over each direction (right, down)
                const nx = x + d.dx, ny = y + d.dy;
                if (nx < width && ny < height) {
                    if (edges.has(edgeKey(x, y, nx, ny)) || edges.has(edgeKey(nx, ny, x, y))) {
                        const ncx = offset_x + nx * cell_size + cell_size / 2;
                        const ncy = offset_y + ny * cell_size + cell_size / 2;
                        ctx.moveTo(cx, cy);
                        ctx.lineTo(ncx, ncy);
                    }
                }
            });
        }
    }
    ctx.stroke();
    // Draw nodes
    const nodeRadius = Math.max(4, cell_size * 0.2);
    ctx.fillStyle = '#ffffff';
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const cx = offset_x + x * cell_size + cell_size / 2;
            const cy = offset_y + y * cell_size + cell_size / 2;
            ctx.beginPath();
            ctx.arc(cx, cy, nodeRadius, 0, 2 * Math.PI);
            ctx.fill();
        }
    }
    // Draw start
    if (start) {
        ctx.fillStyle = '#4ec94e';
        const sx = offset_x + start.x * cell_size + cell_size / 2;
        const sy = offset_y + start.y * cell_size + cell_size / 2;
        ctx.beginPath();
        ctx.arc(sx, sy, nodeRadius * 1.5, 0, 2 * Math.PI);
        ctx.fill();
    }
    // Draw finish
    if (finish) {
        ctx.fillStyle = '#e05252';
        const fx = offset_x + finish.x * cell_size + cell_size / 2;
        const fy = offset_y + finish.y * cell_size + cell_size / 2;
        ctx.beginPath();
        ctx.arc(fx, fy, nodeRadius * 1.5, 0, 2 * Math.PI);
        ctx.fill();
    }
    ctx.setTransform(1, 0, 0, 1, 0, 0);
}
document.getElementById('prevMazeBtn').onclick = () => {
    if (currentMazeIdx > 0) {
        currentMazeIdx--;
        RenderMazeScroll();
    }
};
document.getElementById('nextMazeBtn').onclick = () => {
    if (currentMazeIdx < mazeDataArr.length - 1) {
        currentMazeIdx++;
        RenderMazeScroll();
    };
};
async function refreshLogs() {
    try {
        const res = await fetch('/logs');
        if (!res.ok) return;
        const lines = await res.json();
        const consoleLog = document.getElementById('consoleLog');
        consoleLog.textContent = lines.join('\n');
        consoleLog.scrollTop = consoleLog.scrollHeight;
    } catch (err) {
        console.error('Log polling failed', err);
    }
}
setInterval(refreshLogs, 500);