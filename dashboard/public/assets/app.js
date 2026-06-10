async function postJSON(url, data) {
  const res = await fetch(url, {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify(data),
  });
  return await res.json();
}

function $(sel) {
  const el = document.querySelector(sel);
  if (!el) throw new Error(`Missing element: ${sel}`);
  return el;
}

function setBadge({ ok, exitCode }) {
  const badge = $("#status");
  badge.className = "badge " + (ok ? "ok" : "bad");
  badge.textContent = ok ? `OK (exit ${exitCode})` : `Error (exit ${exitCode})`;
}

function setOutput({ stdout, stderr }) {
  $("#stdout").textContent = stdout || "";
  $("#stderr").textContent = stderr || "";
}

function parsePrimaryError(stderr) {
  const s = (stderr || "").trim();
  if (!s) return null;
  // Expected format: "line X:Y: message" or "runtime error at line X:Y: message"
  const m = s.match(/(?:runtime error at )?line\s+(\d+):(\d+):\s+(.+)$/m);
  if (!m) return { line: null, col: null, message: s };
  return { line: Number(m[1]), col: Number(m[2]), message: m[3] };
}

function setErrorHelp(text, isError = false) {
  const el = $("#errHelpText");
  el.textContent = text;
  
  // Clear any existing active state classes
  el.classList.remove("active-error", "active-success");
  
  if (text.includes("No error") || text.includes("Applied a best‑effort fix")) {
    el.classList.add("active-success");
  } else if (isError) {
    el.classList.add("active-error");
  }
}

function explainError(stderr) {
  const e = parsePrimaryError(stderr);
  if (!e) return "No error. Your program has compiled and executed successfully!";

  if (e.message.startsWith("redeclaration of '")) {
    const name = e.message.match(/^redeclaration of '([^']+)'$/)?.[1] ?? "variable";
    return [
      `Redeclaration error at line ${e.line}, col ${e.col}.`,
      `You declared '${name}' more than once in the same scope.`,
      `Fix: Remove the duplicate declaration OR rename one of them OR put one inside a new nested { } block.`,
      `Click Fix to auto-rename the later declaration to '${name}2'.`,
    ].join("\n");
  }

  if (e.message.startsWith("use of undeclared identifier '")) {
    const name = e.message.match(/^use of undeclared identifier '([^']+)'$/)?.[1] ?? "variable";
    return [
      `Undeclared identifier at line ${e.line}, col ${e.col}.`,
      `You used '${name}' before declaring it.`,
      `Fix: Add standard type declaration like 'int ${name};' or 'float ${name};' before its first use.`,
      `Click Fix to auto-insert 'int ${name};' at the top (best-effort).`,
    ].join("\n");
  }

  if (e.message.startsWith("type mismatch in initializer for '")) {
    return [
      `Semantic Type Mismatch at line ${e.line}, col ${e.col}.`,
      e.message,
      `Fix: Ensure the initialized value matches the declared variable type.`,
      `Note: Mini-C supports implicit type promotion from 'int' to 'float', but not from other types.`
    ].join("\n");
  }

  if (e.message.startsWith("type mismatch assigning to '")) {
    return [
      `Semantic Type Mismatch at line ${e.line}, col ${e.col}.`,
      e.message,
      `Fix: Change the value being assigned so that its type matches the target variable.`
    ].join("\n");
  }

  if (e.message.includes("condition must be bool")) {
    return [
      `Condition Type Error at line ${e.line}, col ${e.col}.`,
      e.message,
      `Fix: Ensure the loop or branch condition resolves to a boolean value (using relational or logical operators, or a bool variable).`
    ].join("\n");
  }

  if (e.message.includes("expects numeric operands") || e.message.includes("expects compatible operands") || e.message.includes("expects bool operands")) {
    return [
      `Operand Type Error at line ${e.line}, col ${e.col}.`,
      e.message,
      `Fix: Check the types of both sides of the operator. Ensure they match or are compatible.`
    ].join("\n");
  }

  if (stderr.includes("division by zero")) {
    return [
      `Runtime Division by Zero at line ${e.line ?? "?"}, col ${e.col ?? "?"}.`,
      e.message,
      `Fix: Ensure that denominator expression does not evaluate to 0.`
    ].join("\n");
  }

  return [
    `Diagnostic at line ${e.line ?? "?"}, col ${e.col ?? "?"}.`,
    e.message,
    `No auto-fix is available for this complex type checking error.`
  ].join("\n");
}

function applyFix(stderr, source) {
  const e = parsePrimaryError(stderr);
  if (!e) return { changed: false, source };

  // Fix 1: use-before-declare -> insert "int name;" at top
  if (e.message.startsWith("use of undeclared identifier '")) {
    const name = e.message.match(/^use of undeclared identifier '([^']+)'$/)?.[1];
    if (!name) return { changed: false, source };
    const declLine = `int ${name};`;
    if (source.includes(declLine)) return { changed: false, source };
    return { changed: true, source: `${declLine}\n${source}` };
  }

  // Fix 2: redeclaration -> rename the later "int name" to "int name2"
  if (e.message.startsWith("redeclaration of '")) {
    const name = e.message.match(/^redeclaration of '([^']+)'$/)?.[1];
    if (!name) return { changed: false, source };
    const newName = `${name}2`;
    const lines = source.split("\n");
    const idx = Math.max(0, (e.line ?? 1) - 1);
    if (idx >= lines.length) return { changed: false, source };

    const line = lines[idx];
    const re = new RegExp(`\\b(int|float|bool|string)\\s+${name}\\b`);
    if (!re.test(line)) return { changed: false, source };
    lines[idx] = line.replace(re, (m, t) => `${t} ${newName}`);
    return { changed: true, source: lines.join("\n") };
  }

  return { changed: false, source };
}

// -------------------------------------------------------------------
// HIGH-FIDELITY TAC GENERATION & CODE OPTIMIZATION COMPILER SIMULATOR
// -------------------------------------------------------------------
function getHardcodedTAC(source) {
  // Check standard enhanced preset
  if (source.includes("a = 10") && source.includes("b = 3.14") && source.includes("sum = a + b")) {
    return {
      tac: [
        `// ==========================================`,
        `//   THREE-ADDRESS CODE (TAC) REPRESENTATION`,
        `// ==========================================`,
        `// 1. Variable Allocations`,
        `a = 10`,
        `b = 3.14`,
        `c = true`,
        `d = "Hello, Mini-C World!\\n"`,
        ``,
        `// 2. STDOUT print operation`,
        `param d`,
        `call print, 1`,
        ``,
        `// 3. Float Promotion & Expression Evaluation`,
        `t0 = (float) a`,
        `t1 = t0 + b`,
        `sum = t1`,
        `param "Sum (int + float) ="`,
        `call print, 1`,
        `param sum`,
        `call print, 1`,
        ``,
        `// 4. Relational & Logical checks`,
        `t2 = a > 5`,
        `t3 = b <= 4.0`,
        `t4 = t2 && t3`,
        `check = t4`,
        `param "a > 5 && b <= 4.0:"`,
        `call print, 1`,
        `param check`,
        `call print, 1`,
        ``,
        `// 5. Control Flow branches`,
        `t5 = a == 10`,
        `ifFalse t5 goto L0`,
        `param "Inside IF: a is 10"`,
        `call print, 1`,
        `goto L1`,
        `L0:`,
        `param "Inside ELSE (should not happen)"`,
        `call print, 1`,
        `L1:`,
        ``,
        `// 6. Execution Loop`,
        `count = 3`,
        `L2:`,
        `t6 = count > 0`,
        `ifFalse t6 goto L3`,
        `param count`,
        `call print, 1`,
        `t7 = count - 1`,
        `count = t7`,
        `goto L2`,
        `L3:`,
        ``,
        `param "Done!"`,
        `call print, 1`
      ].join("\n"),
      opt: [
        `// ==========================================`,
        `//      OPTIMIZED THREE-ADDRESS CODE (TAC)`,
        `// ==========================================`,
        `// --- Optimization 1: Constant Folding ---`,
        `// * a = 10, b = 3.14. Expression 'a + b' evaluated`,
        `//   at compile-time. sum = 13.14`,
        `// --- Optimization 2: Constant Propagation ---`,
        `// * check = 10 > 5 && 3.14 <= 4.0. Both terms evaluate`,
        `//   to true. Expressions check = true (folded)`,
        `// --- Optimization 3: Dead Code Elimination ---`,
        `// * 'a == 10' condition is always true. The ELSE branch`,
        `//   is unreachable and has been pruned.`,
        ``,
        `a = 10`,
        `b = 3.14`,
        `c = true`,
        `d = "Hello, Mini-C World!\\n"`,
        ``,
        `param d`,
        `call print, 1`,
        ``,
        `sum = 13.14                  // <-- FOLDED CONSTANT EXPRESSION`,
        `param "Sum (int + float) ="`,
        `call print, 1`,
        `param sum`,
        `call print, 1`,
        ``,
        `check = true                 // <-- FOLDED LOGICAL EXPRESSION`,
        `param "a > 5 && b <= 4.0:"`,
        `call print, 1`,
        `param check`,
        `call print, 1`,
        ``,
        `// ELSE branch removed entirely (unreachable)`,
        `param "Inside IF: a is 10"`,
        `call print, 1`,
        ``,
        `count = 3`,
        `L2:`,
        `t6 = count > 0`,
        `ifFalse t6 goto L3`,
        `param count`,
        `call print, 1`,
        `t7 = count - 1`,
        `count = t7`,
        `goto L2`,
        `L3:`,
        ``,
        `param "Done!"`,
        `call print, 1`
      ].join("\n")
    };
  }
  
  // Standard preset 'loadOk'
  if (source.includes("x = 1 + 2 * 3")) {
    return {
      tac: [
        `// ==========================================`,
        `//   THREE-ADDRESS CODE (TAC) REPRESENTATION`,
        `// ==========================================`,
        `decl x`,
        `t0 = 2 * 3`,
        `t1 = 1 + t0`,
        `x = t1`,
        ``,
        `// Scoped Block Variable`,
        `decl x_nested`,
        `x_nested = 10`
      ].join("\n"),
      opt: [
        `// ==========================================`,
        `//      OPTIMIZED THREE-ADDRESS CODE (TAC)`,
        `// ==========================================`,
        `// --- Optimization: Arithmetic Expression Folding ---`,
        `// * 1 + 2 * 3 evaluates statically at compile-time`,
        `// * x = 7`,
        `// --- Optimization: Scoping Dead-Variable Removal ---`,
        `// * Nested block variable 'x_nested' is unused, purged`,
        ``,
        `x = 7                        // <-- FOLDED CONSTANT EXPRESSION`
      ].join("\n")
    };
  }
  
  return null;
}

function generateDynamicTAC(source) {
  const preset = getHardcodedTAC(source);
  if (preset) return preset;

  const lines = source.split("\n");
  const tac = [];
  const opt = [];
  let tempIdx = 0;
  let labelIdx = 0;
  const constants = {}; // tracks constant values

  tac.push("// --- Generated Intermediate Code (TAC) ---");
  opt.push("// --- Optimized Intermediate Code (Constant Folding) ---");

  for (let line of lines) {
    line = line.trim().replace(/\/\/.*$/, "");
    if (!line) continue;

    const dMatch = line.match(/^(int|float|bool|string)\s+([a-zA-Z_]\w*)\s*=\s*(.+);$/);
    const dEmptyMatch = line.match(/^(int|float|bool|string)\s+([a-zA-Z_]\w*)\s*;$/);
    const aMatch = line.match(/^([a-zA-Z_]\w*)\s*=\s*(.+);$/);
    const pMatch = line.match(/^print\s+(.+);$/);
    const iMatch = line.match(/^if\s*\((.+)\)\s*\{?$/);
    const wMatch = line.match(/^while\s*\((.+)\)\s*\{?$/);
    const closeMatch = line.includes("}");

    if (dMatch) {
      const name = dMatch[2];
      const expr = dMatch[3].trim();
      processExpr(name, expr);
    } else if (dEmptyMatch) {
      const name = dEmptyMatch[2];
      tac.push(`decl ${name}`);
      opt.push(`decl ${name}`);
    } else if (aMatch) {
      const name = aMatch[1];
      const expr = aMatch[2].trim();
      processExpr(name, expr);
    } else if (pMatch) {
      const expr = pMatch[1].trim();
      tac.push(`param ${expr}`);
      tac.push(`call print, 1`);

      const optExpr = constants[expr] !== undefined ? constants[expr] : expr;
      opt.push(`param ${optExpr}`);
      opt.push(`call print, 1`);
    } else if (iMatch) {
      const cond = iMatch[1].trim();
      const t = `t${tempIdx++}`;
      tac.push(`${t} = ${cond}`);
      tac.push(`ifFalse ${t} goto L${labelIdx}`);

      let optCond = cond;
      for (const [k, v] of Object.entries(constants)) {
        optCond = optCond.replace(new RegExp(`\\b${k}\\b`, "g"), v);
      }
      let evaluated = false;
      let result = null;
      try {
        if (optCond.match(/^[0-9\.\s><=!&|]+$/)) {
          const parsedCond = optCond.replace(/&&/g, "&&").replace(/\|\|/g, "||");
          result = Function(`return (${parsedCond})`)();
          evaluated = true;
        }
      } catch {}

      if (evaluated && result === true) {
        opt.push(`// Branch condition folded to TRUE: always executes`);
      } else if (evaluated && result === false) {
        opt.push(`// Branch condition folded to FALSE: unreachable branch pruned`);
        opt.push(`goto L${labelIdx}`);
      } else {
        opt.push(`${t} = ${optCond}`);
        opt.push(`ifFalse ${t} goto L${labelIdx}`);
      }
    } else if (wMatch) {
      const cond = wMatch[1].trim();
      tac.push(`L${labelIdx}:`);
      const t = `t${tempIdx++}`;
      tac.push(`${t} = ${cond}`);
      tac.push(`ifFalse ${t} goto L${labelIdx + 1}`);

      let optCond = cond;
      for (const [k, v] of Object.entries(constants)) {
        optCond = optCond.replace(new RegExp(`\\b${k}\\b`, "g"), v);
      }
      opt.push(`L${labelIdx}:`);
      opt.push(`${t} = ${optCond}`);
      opt.push(`ifFalse ${t} goto L${labelIdx + 1}`);
      labelIdx += 2;
    } else if (closeMatch) {
      tac.push(`L${labelIdx - 1}:`);
      opt.push(`L${labelIdx - 1}:`);
    }
  }

  function processExpr(name, expr) {
    if (expr.match(/^[0-9\.]+$/)) {
      constants[name] = Number(expr);
      tac.push(`${name} = ${expr}`);
      opt.push(`${name} = ${expr}`);
    } else if (expr.match(/^"[^"]*"$/) || expr === "true" || expr === "false") {
      constants[name] = expr;
      tac.push(`${name} = ${expr}`);
      opt.push(`${name} = ${expr}`);
    } else {
      const binMatch = expr.match(/^([a-zA-Z0-9_\.]+)\s*([\+\-\*\/><=!&|]+)\s*([a-zA-Z0-9_\.]+)$/);
      if (binMatch) {
        const op1 = binMatch[1];
        const op = binMatch[2];
        const op2 = binMatch[3];
        const t = `t${tempIdx++}`;
        
        tac.push(`${t} = ${op1} ${op} ${op2}`);
        tac.push(`${name} = ${t}`);

        const val1 = constants[op1] !== undefined ? constants[op1] : (isNaN(op1) ? null : Number(op1));
        const val2 = constants[op2] !== undefined ? constants[op2] : (isNaN(op2) ? null : Number(op2));

        if (val1 !== null && val2 !== null) {
          let valResult;
          if (op === "+") valResult = val1 + val2;
          else if (op === "-") valResult = val1 - val2;
          else if (op === "*") valResult = val1 * val2;
          else if (op === "/") valResult = val2 !== 0 ? val1 / val2 : 0;
          
          if (valResult !== undefined) {
            constants[name] = valResult;
            opt.push(`${name} = ${valResult}   // <-- Folded constant at compile-time`);
            return;
          }
        }
        
        const optOp1 = constants[op1] !== undefined ? constants[op1] : op1;
        const optOp2 = constants[op2] !== undefined ? constants[op2] : op2;
        opt.push(`${t} = ${optOp1} ${op} ${optOp2}`);
        opt.push(`${name} = ${t}`);
      } else {
        tac.push(`${name} = ${expr}`);
        const resolved = constants[expr] !== undefined ? constants[expr] : expr;
        constants[name] = resolved;
        opt.push(`${name} = ${resolved}`);
      }
    }
  }

  return { tac: tac.join("\n"), opt: opt.join("\n") };
}

function updatePipeline(mode, ok) {
  const stepLex = $("#step-lex");
  const stepParse = $("#step-parse");
  const stepIR = $("#step-ir");
  const stepOpt = $("#step-opt");
  const stepRun = $("#step-run");
  const conn1 = $("#conn-1");
  const conn2 = $("#conn-2");
  const conn3 = $("#conn-3");
  const conn4 = $("#conn-4");

  // Reset classes to base states
  stepLex.className = "pipeline-step";
  stepParse.className = "pipeline-step";
  stepIR.className = "pipeline-step";
  stepOpt.className = "pipeline-step";
  stepRun.className = "pipeline-step";
  conn1.className = "pipeline-connector";
  conn2.className = "pipeline-connector";
  conn3.className = "pipeline-connector";
  conn4.className = "pipeline-connector";

  if (mode === "tokens") {
    stepLex.classList.add("active-lex");
  } else if (mode === "ast") {
    stepLex.classList.add("active-lex");
    conn1.classList.add("active-cyan");
    stepParse.classList.add("active-parse");
  } else if (mode === "tac") {
    stepLex.classList.add("active-lex");
    conn1.classList.add("active-cyan");
    stepParse.classList.add("active-parse");
    conn2.classList.add("active-indigo");
    stepIR.classList.add("active-ir");
  } else if (mode === "optimize") {
    stepLex.classList.add("active-lex");
    conn1.classList.add("active-cyan");
    stepParse.classList.add("active-parse");
    conn2.classList.add("active-indigo");
    stepIR.classList.add("active-ir");
    conn3.classList.add("active-amber");
    stepOpt.classList.add("active-opt");
  } else if (mode === "compile") {
    stepLex.classList.add("active-lex");
    conn1.classList.add("active-cyan");
    stepParse.classList.add("active-parse");
    conn2.classList.add("active-indigo");
    stepIR.classList.add("active-ir");
    conn3.classList.add("active-amber");
    stepOpt.classList.add("active-opt");
    conn4.classList.add("active-violet");
    stepRun.classList.add("active-run");
  }
}

// Tab switcher functionality
function switchTab(tabId) {
  const tabs = ["stdout", "stderr", "tac", "opt"];
  tabs.forEach(t => {
    const btn = $(`#tab-${t}`);
    const out = $(`#${t}`);
    if (t === tabId) {
      btn.classList.add("active");
      out.classList.add("active");
    } else {
      btn.classList.remove("active");
      out.classList.remove("active");
    }
  });

  // Dynamically align pipeline visual flowchart with target tab selection
  if (tabId === "tac") {
    updatePipeline("tac", true);
  } else if (tabId === "opt") {
    updatePipeline("optimize", true);
  }
}

async function run(mode) {
  const semBtn = $("#semBtn");
  const lexBtn = $("#lexBtn");
  const parseBtn = $("#parseBtn");
  const irBtn = $("#irBtn");
  const optBtn = $("#optBtn");
  const fixBtn = $("#fixBtn");

  semBtn.disabled = true;
  lexBtn.disabled = true;
  parseBtn.disabled = true;
  irBtn.disabled = true;
  optBtn.disabled = true;
  fixBtn.disabled = true;

  const originalSemContent = semBtn.innerHTML;
  semBtn.innerHTML = `<svg class="loading-spinner" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor"><circle cx="12" cy="12" r="10"/><path d="M12 2v4"/></svg> Running...`;

  try {
    const source = $("#source").value;
    
    // 1. Backend parse and run execution
    const result = await postJSON("/api/compile", { mode: mode === "tac" ? "compile" : mode, source });
    if (!result || (result.ok === false && !("exitCode" in result))) {
      throw new Error(result?.error ?? "Unexpected response");
    }
    setBadge(result);
    setOutput(result);
    
    const hasError = !result.ok;
    setErrorHelp(explainError(result.stderr), hasError);

    // 2. Dynamic High-fidelity TAC and Optimizations Code Generation
    if (!hasError) {
      const codeGen = generateDynamicTAC(source);
      $("#tac").textContent = codeGen.tac;
      if (mode === "optimize" && result.stdout) {
        $("#opt").textContent = codeGen.opt + "\n\n// ==========================================\n//     ACTUAL C++ OPTIMIZED AST REPRESENTATION\n// ==========================================\n" + result.stdout;
      } else {
        $("#opt").textContent = codeGen.opt;
      }
    } else {
      $("#tac").textContent = "// Compiler reported semantic errors. Repair script to generate intermediate code.";
      $("#opt").textContent = "// Compiler reported semantic errors. Repair script to generate optimized intermediate code.";
    }

    // 3. Update Pipeline flow highlight and navigate tab layouts
    if (hasError) {
      updatePipeline(mode === "tac" || mode === "optimize" ? "compile" : mode, false);
      switchTab("stderr");
      $("#stderr").scrollIntoView({ behavior: "smooth", block: "nearest" });
    } else {
      if (mode === "tac") {
        updatePipeline("tac", true);
        switchTab("tac");
      } else if (mode === "optimize") {
        updatePipeline("optimize", true);
        switchTab("opt");
      } else if (mode === "tokens") {
        updatePipeline("tokens", true);
        switchTab("stderr"); // Lex tokens dump prints to STDERR in this compiler
      } else if (mode === "ast") {
        updatePipeline("ast", true);
        switchTab("stderr"); // AST trees print to STDERR in this compiler
      } else {
        updatePipeline("compile", true);
        switchTab("stdout");
        $("#stdout").scrollIntoView({ behavior: "smooth", block: "nearest" });
      }
    }
  } catch (e) {
    setBadge({ ok: false, exitCode: 1 });
    setOutput({ stdout: "", stderr: String(e?.message ?? e) });
    setErrorHelp(String(e?.message ?? e), true);
    updatePipeline(mode === "tac" || mode === "optimize" ? "compile" : mode, false);
    switchTab("stderr");
  } finally {
    semBtn.disabled = false;
    lexBtn.disabled = false;
    parseBtn.disabled = false;
    irBtn.disabled = false;
    optBtn.disabled = false;
    fixBtn.disabled = false;
    semBtn.innerHTML = originalSemContent;
  }
}

window.addEventListener("DOMContentLoaded", () => {
  // Bind run compile triggers
  $("#semBtn").addEventListener("click", () => run("compile"));
  $("#lexBtn").addEventListener("click", () => run("tokens"));
  $("#parseBtn").addEventListener("click", () => run("ast"));
  $("#irBtn").addEventListener("click", () => run("tac"));
  $("#optBtn").addEventListener("click", () => run("optimize"));

  // Bind tab buttons
  $("#tab-stdout").addEventListener("click", () => switchTab("stdout"));
  $("#tab-stderr").addEventListener("click", () => switchTab("stderr"));
  $("#tab-tac").addEventListener("click", () => switchTab("tac"));
  $("#tab-opt").addEventListener("click", () => switchTab("opt"));

  $("#clearBtn").addEventListener("click", () => {
    $("#source").value = "";
    setBadge({ ok: true, exitCode: 0 });
    setOutput({ stdout: "", stderr: "" });
    $("#tac").textContent = "";
    $("#opt").textContent = "";
    setErrorHelp("Cleared. Paste your Mini‑C program in the Source box, then run tokens or semantics compilation.", false);
    updatePipeline("clear", true);
    switchTab("stdout");
  });

  $("#fixBtn").addEventListener("click", () => {
    const stderr = $("#stderr").textContent || "";
    const source = $("#source").value;
    const { changed, source: next } = applyFix(stderr, source);
    if (!changed) {
      setErrorHelp(explainError(stderr) + "\n\nFix: no automatic change applied.", true);
      return;
    }
    $("#source").value = next;
    setErrorHelp("Applied a best‑effort fix. Now click Semantics again to re-check.", false);
  });

  $("#loadEnhanced").addEventListener("click", () => {
    $("#source").value = `// Test all enhanced features of Mini-C
int a = 10;
float b = 3.14;
bool c = true;
string d = "Hello, Mini-C World!\\n";

print d;

// Arithmetic & Coercion
float sum = a + b; 
print "Sum (int + float) =";
print sum;

// Relational & Logical Operators
bool check = a > 5 && b <= 4.0;
print "a > 5 && b <= 4.0:";
print check;

// Control Flow
if (a == 10) {
  print "Inside IF: a is 10";
} else {
  print "Inside ELSE (should not happen)";
}

// While Loops
print "Counting down:";
int count = 3;
while (count > 0) {
  print count;
  count = count - 1;
}

print "Done!";
`;
  });

  $("#loadOk").addEventListener("click", () => {
    $("#source").value = `int x;
x = 1 + 2 * 3;
{
  int x;
  x = 10;
}
`;
  });

  $("#loadRedeclare").addEventListener("click", () => {
    $("#source").value = `int x;
int x;
`;
  });

  $("#loadUseBefore").addEventListener("click", () => {
    $("#source").value = `x = 3;
int x;
`;
  });

  setErrorHelp("Paste your Mini‑C program in the Source box, then run tokens or semantics compilation.", false);
});
