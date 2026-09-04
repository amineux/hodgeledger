# Harmonic 1-chains of a citation complex

## The question

ClaimLedger asked which *papers* hold two fields together: a Fiedler cut of
the citation graph. HodgeLedger asks a strictly richer question. Science is
not only a graph of credit. It is a **complex of agreements**. Three papers
that all pairwise couple form a filled triangle: a 2-simplex. A cycle that
can be written as a sum of those triangles is *paid* — it is a boundary.
A cycle that cannot is **circular intellectual debt**: credit that flows
around a loop which no local co-citation fills.

**Problem (analytic).**
Let \(K\) be a finite simplicial complex of dimension \(\le 2\) whose
0-simplices are papers, 1-simplices are undirected coupling/citation edges,
and 2-simplices are 3-cliques of that graph (the **clique complex** / flag
complex). Write \(C_k\) for real \(k\)-chains and \(\partial_k: C_k\to C_{k-1}\)
the boundary. The **Hodge Laplacian** on \(k\)-cochains is

\[
L_k
 \;=\;
 \partial_k^\top \partial_k
 \;+\;
 \partial_{k+1}\partial_{k+1}^\top
 \;=\;
 B_k^\top B_k
 \;+\;
 B_{k+1} B_{k+1}^\top.
\]

Approximate

\[
\mathcal{H}_1
 \;=\;
 \ker L_1
 \;=\;
 \ker\partial_1 \;\cap\; \ker\partial_2^\top
 \;\cong\;
 H_1(K;\mathbb{R}),
\]

the harmonic 1-chains, and **rank** those cycles by Rayleigh energy
\(f^\top L_1 f\) (zero on the nose for true harmonics) and by
cross-field participation of \(|f|\). Combinatorial homology of a 1500-vertex
flag complex is possible but uninformative as a *ranking* and expensive as a
pipeline. HodgeLedger ships a **sparse Lanczos-on-\(L_1\)** heuristic with
planted-cycle tests and a ClaimLedger Fiedler baseline on the same 0-skeleton.

## Boundary operators

Orient every 1-simplex \(\{u,v\}\) as \(u\to v\) with \(u<v\) (combinatorial,
stable). Orient every 2-simplex \(\{a,b,c\}\) with \(a<b<c\). Then, in the
ordered bases of vertices / edges / triangles,

\[
(B_1)_{w,e}
 \;=\;
 \begin{cases}
  -1 & w=\mathrm{src}(e)\\
  +1 & w=\mathrm{tgt}(e)\\
   0 & \text{otherwise}
 \end{cases}
\qquad
(B_2)_{e,\sigma}
 \;=\;
 \begin{cases}
  +1 & e=[a,b]\text{ or }[b,c]\\
  -1 & e=[a,c]\\
   0 & \text{otherwise}
 \end{cases}
\]

for \(\sigma=[a,b,c]\). This is \(\partial[a,b,c]=[b,c]-[a,c]+[a,b]\).
The simplicial identity \(B_1 B_2=0\) is a unit test, not an assumption.

Consequently

\[
L_0 = B_1 B_1^\top = D-A
\]

is the combinatorial graph Laplacian of the 0-skeleton, and

\[
L_1 = B_1^\top B_1 + B_2 B_2^\top
\]

is the **graph Helmholtzian** plus the **triangle up-term**. The first
summand scores how far a 1-cochain is from being a cycle; the second scores
how far it is from being orthogonal to boundaries. Harmonic means both.

On fixtures small enough that \(n_2\) is modest we also form
\(L_2=B_2^\top B_2\) (no 3-simplices).

## Hodge decomposition

For a finite simplicial complex with real coefficients,

\[
C_1
 \;=\;
 \mathrm{im}\,\partial_2
 \;\oplus\;
 \mathcal{H}_1
 \;\oplus\;
 \mathrm{im}\,\partial_1^\top.
\]

- **Gradient** 1-cochains \(\mathrm{im}\,B_1^\top\): potential differences
  between papers. ClaimLedger’s Fiedler vector, pulled back to edges, lives
  here.
- **Curl / boundary** 1-cochains \(\mathrm{im}\,B_2\): filled triangles.
  A 3-cycle around a co-citation triangle is *accounted for*.
- **Harmonic** 1-cochains \(\ker L_1\): cycles that are not boundaries.
  These are the objects HodgeLedger ranks.

Betti numbers: \(\beta_0=\dim\ker L_0\) (connected components),
\(\beta_1=\dim\ker L_1\). The pipeline reports the count of \(L_1\) Ritz
values below `harmonic_tol` (\(10^{-6}\)) as \(\tilde\beta_1\).

## Lanczos heuristic

\(L_1\) is real symmetric positive semidefinite of order \(n_1\) (the number
of undirected edges). We never form dense matrices. \(L_0\) and \(L_1\) are
assembled as CSR from the Gram products \(B B^\top\) / \(B^\top B\).

**Lanczos** (self-contained, no Eigen):

1. Krylov subspace of dimension
   \(s=\min(n,\max(4k+16,64))\) (192 steps when \(n>400\), capped at 256)
   against \(L_k\). Numerical \(\ker L_1\) uses \(\lambda<10^{-4}\).
2. **Full reorthogonalization**, two passes
   (Daniel–Gragg–Kaufman–Stewart).
3. Dense Jacobi eigen-decomposition of the projected tridiagonal
   (Rayleigh–Ritz).
4. If a \(\beta_j\) breaks down, spawn a fresh Gaussian vector orthogonal
   to \(Q\) so repeated eigenvalues (multiplicity of the kernel, \(C_6\)’s
   double 1, \(K_5\)’s quadruple 5) are not missed.
5. Report residuals \(\|Lq-\lambda q\|\).

This is the same *quality bar* as ClaimLedger’s eigensolver, applied to
both \(L_0\) (paper embedding) and \(L_1\) (edge / harmonic space).

A Ritz pair with \(\lambda<\texttt{harmonic\_tol}\) is declared harmonic.
The corresponding unit 1-cochain \(f\) is thresholded at a fraction of
\(\|f\|_\infty\); the support subgraph is traced into a loop; edges are
ranked inside the cycle by \(|f_e|\).

**Ranking.** Among extracted modes, sort by \(\lambda\) ascending, then by
cross-field mass \(\sum_{c(u)\ne c(v)}|f_{uv}|\), then by Shannon entropy of
\(|f|\) across fields. Energy of a unit vector is exactly \(\lambda\).

## Comparison to the ClaimLedger Fiedler baseline

On the **same 0-skeleton** we also assemble the symmetric normalized
Laplacian

\[
L_{\mathrm{sym}} = I - D^{-1/2} A D^{-1/2}
\]

(isolates get a zero row) and report its first nontrivial eigenvalue
\(\lambda_2(L_{\mathrm{sym}})\) next to Hodge \(\lambda_2(L_0)\). They are
not the same number: one is normalized Dirichlet energy, the other is
combinatorial. The atlas embedding uses Hodge \(L_0\) so that the geometry
is the actual 0-Hodge operator; `graph_meta.json` keeps the Fiedler value
so a ClaimLedger-style cut reading remains available.

Fiedler finds *thin vertex cuts*. Harmonic 1-chains find *unfilled loops*.
A planted chordless cycle across two fields is invisible to a vertex-cut
score if the graph is otherwise a tree of blobs (algebraic connectivity
is then a path-bottleneck, not a cycle). HodgeLedger’s unit test is
exactly that situation.

## Simplicial cuts: leave-one-edge-out

Exact \(\arg\max_{|S|\le k}\delta\lambda\) for \(k>1\) is NP-hard spectral
interdiction. For \(k=1\) on edges we **solve a shortlist exactly**:

1. Pre-filter: the \(P\) edges of largest \(|f_e|\) on the leading harmonic
   mode (\(P=\min(n_1,32)\); on the tiny fixture, every edge).
2. For each candidate \(e\), delete \(e\) and every triangle containing it,
   rebuild \(B_1,B_2,L_1\), recompute \(\lambda_{\min}(L_1)\).
3. \(\delta\lambda(e)=\lambda_{\min}(L_1\setminus e)-\lambda_{\min}(L_1)\).

Deleting an edge of a unique harmonic cycle destroys \(\ker L_1\), so
\(\lambda_{\min}\) jumps from \(\approx 0\) to the Hodge gap. Deleting a
tree-like edge leaves the kernel intact. That is the planted-cycle LOO
test.

Leave-one-triangle-out is the dual experiment (unfilling a triangle *creates*
homology). The shipped CLI implements the edge variant; `without_triangle`
exists on the complex for the dual rebuild.

## Complexity

| object | cost |
|--------|------|
| clique triangles | \(O(\sum_v d_v^2)\) |
| CSR \(B_1,B_2\) | \(O(n_0+n_1+n_2)\) |
| assemble \(L_0\) | \(O(n_1)\) Gram of 2-sparse columns |
| assemble \(L_1\) | \(O(\sum_v d_v^2 + n_2)\) |
| Lanczos, \(s\) steps, full reorth. | \(O(s\,\mathrm{nnz}(L)+s^2 n+s^3)\) |
| leave-one-edge-out, \(P\) candidates | \(P\) rebuilds + \(P\) Lanczos |
| timeline, \(T\) years | \(T\) small solves |
| Smith normal form / exact \(H_1\) | cubic in \(n_1\) over \(\mathbb{Z}\); not shipped |

On the 1600-paper stacked fixture, \(n_1\sim 5\cdot 10^3\), \(n_2\sim 5\cdot 10^3\),
\(s\le 64\), laptop-seconds. The expensive term is \(P\) LOO rebuilds, not
the embedding.

## Accounting of the same chains

**0-flow.** Directed citation \(u\to v\):

```
DR  0:u     1.00   intellectual debt
CR  0:v     1.00   intellectual capital
```

**1-flow.** Harmonic circulation \(f\in\ker L_1\), oriented with the sign of
\(f_e\):

```
DR  1:src(e)    |f_e| scaled
CR  1:tgt(e)    |f_e| scaled
```

Because \(B_1 f=0\), every vertex’s 1-flow trial balance nets to (near) zero:
harmonicity *is* a conservation law. Chart 0 does not have that property —
hubs accumulate capital. The atlas ledger panel shows both.

## Fixtures and planted homology

- `data/fixtures/tiny/` — 12 papers, two filled \(K_4\), a spanning bridge, a chordless
  \(C_4\) on `synth-0009–0010–0011–0012` hanging off a single stem. Tests require the leading \(L_1\)
  mode to put majority mass on those four edges, and the unique largest
  LOO \(\Delta\lambda\) to be one of them.
- `data/fixtures/` — eight stacked chordal fields (each a ball: \(H_1=0\)),
  a spanning path of blobs (connected 0-skeleton, still \(H_1=0\)), then a
  planted unfilled \(C_{12}\) across `cs` and `qbio`. \(n\ge 1500\),
  ids `synth-NNNN`.
- `data/fixtures/arxiv-slice/` — committed public Atom snapshot. Edges are
  author/cross-list **coupling**. No forged famous-paper citations.

## What this does not claim

Lanczos-on-\(L_1\) approximates the *harmonic space*, not a canonical
integral cycle basis. Near-zero Ritz values on a large random flag complex
can mix several loops. Ranking by cross-field mass is a scientific prior,
not a theorem. Exact \(\beta_1\) over \(\mathbb{Z}\) may differ from
\(\tilde\beta_1\) over \(\mathbb{R}\) when torsion is present; the clique
complexes shipped here are torsion-free in practice. The Fiedler baseline
answers a different question and should not be “validated” by matching
Hodge \(\lambda_2\).
