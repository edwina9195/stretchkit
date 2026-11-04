#!/usr/bin/env python3
"""Builds StretchKit_Manual_EN.pdf and StretchKit_Manual_KO.pdf.
usage: build.py <work-dir> <repo-dir> <out-dir>"""
import sys
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.fonts import addMapping
from reportlab.platypus import (BaseDocTemplate, Frame, Image, KeepTogether, NextPageTemplate, PageBreak,
                                PageTemplate, Paragraph, Spacer, Table, TableStyle)

WORK, REPO, OUT = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
sys.path.insert(0, str(WORK))
import content  # noqa: E402

VIOLET = colors.HexColor("#4b4fd6")
DEEP = colors.HexColor("#1d1e5a")
PINK = colors.HexColor("#ff4fd8")
PALE = colors.HexColor("#f1f2ff")
LINE = colors.HexColor("#d5d7fb")
TEXT = colors.HexColor("#1b1c3a")
DIM = colors.HexColor("#6a6d99")

F = WORK / "fonts"
for name, file in [("Lato", "Lato-Regular.ttf"), ("Lato-Bold", "Lato-Bold.ttf"), ("Lato-Black", "Lato-Black.ttf"),
                   ("Nanum", "NanumGothic-Regular.ttf"), ("Nanum-Bold", "NanumGothic-Bold.ttf"),
                   ("Nanum-XBold", "NanumGothic-ExtraBold.ttf"), ("Michroma", "Michroma-Regular.ttf")]:
    pdfmetrics.registerFont(TTFont(name, str(F / file)))
for fam, reg, bold in [("Lato", "Lato", "Lato-Bold"), ("Nanum", "Nanum", "Nanum-Bold")]:
    addMapping(fam, 0, 0, reg); addMapping(fam, 1, 0, bold); addMapping(fam, 0, 1, reg); addMapping(fam, 1, 1, bold)

ENTITIES = {"&mdash;": "—", "&ldquo;": "“", "&rdquo;": "”", "&plusmn;": "±"}


def fix(text):
    for k, v in ENTITIES.items():
        text = text.replace(k, v)
    return text


def build(c, out_path):
    ko = c["lang"] == "ko"
    body_font, bold_font, heavy_font = ("Nanum", "Nanum-Bold", "Nanum-XBold") if ko else ("Lato", "Lato-Bold", "Lato-Black")
    lead = 1.62 if ko else 1.5

    def style(name, **kw):
        base = dict(fontName=body_font, fontSize=9.6, leading=9.6 * lead, textColor=TEXT, spaceAfter=5)
        base.update(kw)
        return ParagraphStyle(name, **base)

    S = {
        "body": style("body"),
        "small": style("small", fontSize=8.4, leading=8.4 * lead, textColor=DIM),
        "cell": style("cell", fontSize=8.6, leading=8.6 * 1.45, spaceAfter=0),
        "cellh": style("cellh", fontName=bold_font, fontSize=8.6, leading=8.6 * 1.45, textColor=colors.white, spaceAfter=0),
        "h": style("h", fontName=heavy_font if ko else "Michroma", fontSize=17 if ko else 14.5, leading=24,
                   textColor=DEEP, spaceAfter=0),
        "num": style("num", fontName="Michroma", fontSize=11, leading=24, textColor=PINK, spaceAfter=0),
        "h2": style("h2", fontName=bold_font, fontSize=11, leading=16, textColor=VIOLET, spaceBefore=6, spaceAfter=4),
        "bullet": style("bullet", leftIndent=12, bulletIndent=2, spaceAfter=3),
        "code": ParagraphStyle("code", fontName="Courier", fontSize=7.9, leading=11.5, textColor=colors.white),
        "toc": style("toc", fontSize=10.5, leading=19),
        "cap": style("cap", fontName=bold_font, fontSize=9.5, leading=13, textColor=DEEP, spaceAfter=1),
        "capdim": style("capdim", fontSize=8.3, leading=11.5, textColor=DIM, spaceAfter=0),
    }
    P = lambda t, s="body": Paragraph(fix(t), S[s])
    W = A4[0] - 40 * mm
    story = []
    sec = {"n": 0}

    def heading(title):
        sec["n"] += 1
        t = Table([[P(f"{sec['n']:02d}", "num"), P(title, "h")]], colWidths=[14 * mm, W - 14 * mm])
        t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "BOTTOM"), ("LEFTPADDING", (0, 0), (-1, -1), 0),
                               ("BOTTOMPADDING", (0, 0), (-1, -1), 4), ("LINEBELOW", (0, 0), (-1, 0), 1.2, VIOLET)]))
        return [Spacer(1, 4), t, Spacer(1, 8)]

    def table(rows, widths, header=True):
        data = [[P(str(cell), "cellh" if (header and r == 0) else "cell") for cell in row] for r, row in enumerate(rows)]
        t = Table(data, colWidths=widths, repeatRows=1 if header else 0)
        st = [("VALIGN", (0, 0), (-1, -1), "TOP"), ("GRID", (0, 0), (-1, -1), 0.5, LINE),
              ("TOPPADDING", (0, 0), (-1, -1), 4.5), ("BOTTOMPADDING", (0, 0), (-1, -1), 4.5),
              ("LEFTPADDING", (0, 0), (-1, -1), 6), ("RIGHTPADDING", (0, 0), (-1, -1), 6)]
        if header:
            st.append(("BACKGROUND", (0, 0), (-1, 0), VIOLET))
        for r in range(1 if header else 0, len(rows)):
            if r % 2 == 0:
                st.append(("BACKGROUND", (0, r), (-1, r), PALE))
        t.setStyle(TableStyle(st))
        return t

    def code(text):
        lines = [Paragraph(l.replace(" ", "&nbsp;"), S["code"]) for l in text.split("\n")]
        t = Table([[lines]], colWidths=[W])
        t.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), DEEP), ("LEFTPADDING", (0, 0), (-1, -1), 9),
                               ("RIGHTPADDING", (0, 0), (-1, -1), 9), ("TOPPADDING", (0, 0), (-1, -1), 7),
                               ("BOTTOMPADDING", (0, 0), (-1, -1), 7)]))
        return [t, Spacer(1, 7)]

    def note(text):
        t = Table([[P(text, "body")]], colWidths=[W])
        t.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), PALE), ("LINEBEFORE", (0, 0), (0, -1), 3, PINK),
                               ("LEFTPADDING", (0, 0), (-1, -1), 10), ("RIGHTPADDING", (0, 0), (-1, -1), 10),
                               ("TOPPADDING", (0, 0), (-1, -1), 7), ("BOTTOMPADDING", (0, 0), (-1, -1), 3)]))
        return [t, Spacer(1, 8)]

    def bullets(items, numbered=False):
        return [Paragraph(fix(t), S["bullet"], bulletText=(f"{i + 1}." if numbered else "•"))
                for i, t in enumerate(items)]

    def img(path, width):
        from PIL import Image as PImage
        w, h = PImage.open(path).size
        return Image(str(path), width=width, height=width * h / w)

    # ---------------------------------------------------------------- page 2: contents + about
    story += [NextPageTemplate("inner"), PageBreak()]
    story.append(P(c["contents"], "h2"))
    toc = [[P(f"<font name='Michroma' color='#ff4fd8'>{i + 1:02d}</font>&nbsp;&nbsp;&nbsp;{t}", "toc")]
           for i, t in enumerate(c["toc"])]
    half = (len(toc) + 1) // 2
    tt = Table([[toc[i][0], toc[i + half][0] if i + half < len(toc) else ""] for i in range(half)], colWidths=[W / 2, W / 2])
    tt.setStyle(TableStyle([("LEFTPADDING", (0, 0), (-1, -1), 0), ("TOPPADDING", (0, 0), (-1, -1), 0),
                            ("BOTTOMPADDING", (0, 0), (-1, -1), 0)]))
    story += [tt, Spacer(1, 14)]

    story += heading(c["about_title"])
    story += [P(c["about"])] + bullets(c["features"])
    story.append(P(c["req_title"], "h2"))
    story += [table(c["req"], [22 * mm, (W - 22 * mm) / 2, (W - 22 * mm) / 2]), Spacer(1, 8)]
    story += note(c["no_vst2"])

    # ---------------------------------------------------------------- download + mac
    story.append(PageBreak())
    story += heading(c["dl_title"])
    story.append(P(c["dl"]))
    story.append(P(f"<link href='{c['dl_link']}' color='#4b4fd6'><u>{c['dl_link']}</u></link>", "body"))
    story += [Spacer(1, 4), table(c["dl_files"], [62 * mm, W - 62 * mm]), Spacer(1, 10)]

    story += heading(c["mac_title"])
    story += bullets(c["mac_steps"], numbered=True)
    story += [table(c["mac_paths"], [24 * mm, 44 * mm, W - 68 * mm]), Spacer(1, 8)]
    story += [Paragraph(fix(c["mac_unblock"]), S["bullet"], bulletText="3.")]
    story += code(c["mac_cmds"])
    story += [Paragraph(fix(c["mac_after"]), S["bullet"], bulletText="4.")]
    story += code(c["mac_cmd2"])

    # ---------------------------------------------------------------- windows + daw
    story.append(PageBreak())
    story += heading(c["win_title"])
    story += bullets(c["win_steps"], numbered=True)
    story += note(c["win_note"])
    story += heading(c["daw_title"])
    story.append(P(c["daw_intro"]))
    story += [table(c["daw"], [36 * mm, W - 36 * mm]), Spacer(1, 6)]

    # ---------------------------------------------------------------- quick start + overview
    story.append(PageBreak())
    story += heading(c["qs_title"])
    story += bullets(c["qs"], numbered=True)
    story.append(PageBreak())
    story += heading(c["ov_title"])
    story += [img(WORK / "overview.png", W), Spacer(1, 8)]
    legend = [P(f"<font name='Michroma' color='#ff4fd8'>{i + 1}</font>&nbsp;&nbsp;{t}", "cell") for i, t in enumerate(c["ov"])]
    half = (len(legend) + 1) // 2
    lt = Table([[legend[i], legend[i + half] if i + half < len(legend) else ""] for i in range(half)],
               colWidths=[W / 2, W / 2])
    lt.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 2),
                            ("TOPPADDING", (0, 0), (-1, -1), 2.5), ("BOTTOMPADDING", (0, 0), (-1, -1), 2.5)]))
    story.append(lt)

    # ---------------------------------------------------------------- map
    story.append(PageBreak())
    story += heading(c["map_title"])
    story += [P(t) for t in c["map_text"]]
    story += [Spacer(1, 4), table(c["map_controls"], [70 * mm, W - 70 * mm]), Spacer(1, 8)]
    story += note(c["map_drop"])

    # ---------------------------------------------------------------- similarity bar
    story.append(PageBreak())
    story += heading(c["sim_title"])
    story.append(P(c["sim_text"]))
    from PIL import Image as PImage
    bar = PImage.open(REPO / "docs" / "skin_E_liquid_chrome.png").crop((0, 60, 1180, 470))
    bar.save(WORK / "simbar.png")
    story += [img(WORK / "simbar.png", W), Spacer(1, 8)]
    story += [table(c["sim_controls"], [36 * mm, W - 36 * mm]), Spacer(1, 12)]

    # ---------------------------------------------------------------- stretch
    story += heading(c["st_title"])
    story.append(P(c["st_text"]))
    story += [Spacer(1, 2), table(c["st_modes"], [30 * mm, W - 30 * mm - 52 * mm, 52 * mm]), Spacer(1, 8)]
    story += note(c["st_note"])

    # ---------------------------------------------------------------- sequencer
    story.append(PageBreak())
    story += heading(c["seq_title"])
    story += [table(c["seq_steps"], [62 * mm, W - 62 * mm]), Spacer(1, 10)]
    story += [table(c["seq_transport"], [30 * mm, W - 30 * mm]), Spacer(1, 10)]
    story += note(c["seq_midi"])

    # ---------------------------------------------------------------- skins
    story.append(PageBreak())
    story += heading(c["sk_title"])
    story.append(P(c["sk_text"]))
    cw = (W - 8 * mm) / 3
    cells = []
    for file, name, desc in c["skins"]:
        cells.append([img(REPO / "docs" / file, cw), Spacer(1, 3), P(name, "cap"), P(desc, "capdim")])
    rows = [[cells[i + k] if i + k < len(cells) else "" for k in range(3)] for i in range(0, len(cells), 3)]
    gt = Table(rows, colWidths=[cw + 4 * mm] * 3)
    gt.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0),
                            ("BOTTOMPADDING", (0, 0), (-1, -1), 9)]))
    story.append(gt)

    # ---------------------------------------------------------------- troubleshooting + licence
    story.append(PageBreak())
    story += heading(c["ts_title"])
    for q, a in c["ts"]:
        story.append(KeepTogether([P(f"<b>{q}</b>", "body"), P(a, "small"), Spacer(1, 2)]))
    story.append(Spacer(1, 8))
    story += heading(c["lic_title"])
    story += [P(t, "small") for t in c["lic"]]
    story.append(Spacer(1, 10))
    story += note(f"{c['credit_line']} <b><link href='{content.INSTA}' color='#4b4fd6'>@sxdrv.mp3</link></b>"
                  f"&nbsp;&nbsp;·&nbsp;&nbsp;<link href='{content.INSTA}' color='#4b4fd6'>instagram.com/sxdrv.mp3</link>")

    # ---------------------------------------------------------------- page decorations
    def cover(canvas, doc):
        pw, ph = A4
        canvas.saveState()
        canvas.drawImage(str(WORK / "cover.jpg"), 0, 0, pw, ph)
        canvas.setFillColor(colors.Color(0.06, 0.05, 0.3, 0.35))
        canvas.setFont("Michroma", 40)
        canvas.drawCentredString(pw / 2 + 1.5, ph * 0.74 - 2, "STRETCHKIT")
        canvas.setFillColor(colors.white)
        canvas.drawCentredString(pw / 2, ph * 0.74, "STRETCHKIT")
        canvas.setFont(body_font, 11.5)
        canvas.drawCentredString(pw / 2, ph * 0.74 - 28, fix(c["cover_tag"]))
        canvas.setFont(bold_font, 15)
        canvas.drawCentredString(pw / 2, ph * 0.66, fix(c["cover_doc"]).replace("&amp;", "&"))
        canvas.setFont(body_font, 9.5)
        canvas.setFillColor(colors.Color(1, 1, 1, 0.8))
        canvas.drawCentredString(pw / 2, ph * 0.66 - 18, fix(c["cover_ver"]).replace("&amp;", "&"))
        # credit
        canvas.setFillColor(colors.white)
        canvas.setFont(body_font, 10)
        canvas.drawCentredString(pw / 2, 30 * mm, c["made_by"])
        canvas.setFont("Michroma", 17)
        canvas.drawCentredString(pw / 2, 30 * mm - 22, "@sxdrv.mp3")
        tw = pdfmetrics.stringWidth("@sxdrv.mp3", "Michroma", 17)
        canvas.linkURL(content.INSTA, (pw / 2 - tw / 2, 30 * mm - 28, pw / 2 + tw / 2, 30 * mm - 4), relative=0)
        canvas.restoreState()

    def inner(canvas, doc):
        pw, ph = A4
        canvas.saveState()
        canvas.setFillColor(VIOLET)
        canvas.rect(0, ph - 5, pw, 5, stroke=0, fill=1)
        canvas.setFillColor(PINK)
        canvas.rect(0, ph - 5, pw * 0.18, 5, stroke=0, fill=1)
        canvas.setFont("Michroma", 7.5)
        canvas.setFillColor(VIOLET)
        canvas.drawString(20 * mm, ph - 13 * mm, "STRETCHKIT")
        canvas.setFont(body_font, 7.8)
        canvas.setFillColor(DIM)
        canvas.drawString(20 * mm, 11 * mm, c["footer"])
        canvas.drawRightString(pw - 20 * mm, 11 * mm, str(doc.page))
        canvas.setStrokeColor(LINE)
        canvas.line(20 * mm, 15 * mm, pw - 20 * mm, 15 * mm)
        canvas.restoreState()

    doc = BaseDocTemplate(str(out_path), pagesize=A4, leftMargin=20 * mm, rightMargin=20 * mm, topMargin=20 * mm,
                          bottomMargin=20 * mm, title="StretchKit — " + fix(c["cover_doc"]).replace("&amp;", "&"),
                          author="sxdrv.mp3", subject="StretchKit v0.1.0")
    frame = Frame(20 * mm, 20 * mm, W, A4[1] - 40 * mm, id="f", leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)
    doc.addPageTemplates([PageTemplate("cover", [frame], onPage=cover), PageTemplate("inner", [frame], onPage=inner)])
    doc.build(story)


OUT.mkdir(parents=True, exist_ok=True)
build(content.EN, OUT / "StretchKit_Manual_EN.pdf")
build(content.KO, OUT / "StretchKit_Manual_KO.pdf")
print("built")
