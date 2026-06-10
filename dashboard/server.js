import http from "node:http";
import { spawn } from "node:child_process";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const HOST = process.env.HOST ?? "127.0.0.1";
const PORT = Number(process.env.PORT ?? 5173);

const projectRoot = path.resolve(__dirname, "..");
const publicDir = path.join(__dirname, "public");
const minicPath = path.join(projectRoot, "build", "minic");

function send(res, status, headers, body) {
  res.writeHead(status, headers);
  res.end(body);
}

function contentType(filePath) {
  if (filePath.endsWith(".html")) return "text/html; charset=utf-8";
  if (filePath.endsWith(".css")) return "text/css; charset=utf-8";
  if (filePath.endsWith(".js")) return "text/javascript; charset=utf-8";
  if (filePath.endsWith(".svg")) return "image/svg+xml";
  return "application/octet-stream";
}

async function readJson(req) {
  const chunks = [];
  for await (const c of req) chunks.push(c);
  const raw = Buffer.concat(chunks).toString("utf8");
  return raw.length ? JSON.parse(raw) : {};
}

async function runMinic({ mode, source }) {
  const tmpDir = await fs.mkdtemp(path.join(os.tmpdir(), "minic-"));
  const filePath = path.join(tmpDir, "input.mc");
  await fs.writeFile(filePath, source ?? "", "utf8");

  const args = [];
  if (mode === "tokens") args.push("--tokens");
  else if (mode === "ast") args.push("--ast");
  else if (mode === "optimize") args.push("--optimize");
  args.push(filePath);

  return await new Promise((resolve) => {
    const child = spawn(minicPath, args, { cwd: projectRoot });
    let stdout = "";
    let stderr = "";
    child.stdout.on("data", (d) => (stdout += d.toString("utf8")));
    child.stderr.on("data", (d) => (stderr += d.toString("utf8")));
    child.on("close", async (code) => {
      // best-effort cleanup
      try {
        await fs.rm(tmpDir, { recursive: true, force: true });
      } catch {}
      resolve({ code: code ?? 1, stdout, stderr });
    });
  });
}

const server = http.createServer(async (req, res) => {
  try {
    if (req.method === "GET" && (req.url === "/" || req.url === "/index.html")) {
      const html = await fs.readFile(path.join(publicDir, "index.html"));
      return send(res, 200, { "content-type": "text/html; charset=utf-8" }, html);
    }

    if (req.method === "GET" && req.url?.startsWith("/assets/")) {
      const rel = req.url.slice("/assets/".length);
      const safe = rel.replaceAll("..", "");
      const filePath = path.join(publicDir, "assets", safe);
      const data = await fs.readFile(filePath);
      return send(res, 200, { "content-type": contentType(filePath) }, data);
    }

    if (req.method === "POST" && req.url === "/api/compile") {
      const body = await readJson(req);
      const mode = body?.mode ?? "compile";
      const source = body?.source ?? "";

      const stat = await fs.stat(minicPath).catch(() => null);
      if (!stat) {
        return send(
          res,
          500,
          { "content-type": "application/json; charset=utf-8" },
          JSON.stringify({
            ok: false,
            error:
              "Mini-C binary not found at ./build/minic. Run `make` in the project root first.",
          }),
        );
      }

      const result = await runMinic({ mode, source });
      return send(
        res,
        200,
        { "content-type": "application/json; charset=utf-8" },
        JSON.stringify({
          ok: result.code === 0,
          exitCode: result.code,
          stdout: result.stdout,
          stderr: result.stderr,
        }),
      );
    }

    send(res, 404, { "content-type": "text/plain; charset=utf-8" }, "Not found\n");
  } catch (e) {
    send(
      res,
      500,
      { "content-type": "text/plain; charset=utf-8" },
      `Server error: ${e?.message ?? String(e)}\n`,
    );
  }
});

server.listen(PORT, HOST, () => {
  // eslint-disable-next-line no-console
  console.log(`Mini-C Dashboard running at http://${HOST}:${PORT}`);
  // eslint-disable-next-line no-console
  console.log(`Project root: ${projectRoot}`);
});

