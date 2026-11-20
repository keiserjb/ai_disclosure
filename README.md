# AI Disclosure

Record how AI was used to create or edit Backdrop content. Show a notice to readers, add source information to the page metadata, and review disclosure records in an admin report.

---

## Overview

The module helps publishers document AI use and human review. It can support a disclosure workflow related to EU AI Act Article 50. Site owners determine which obligations apply and how to meet them.

### Key Capabilities

- **Level Taxonomy:** Tracks 4 standard disclosure tiers:
  1. `human_only`: Authored entirely without generative AI tools.
  2. `ai_assisted`: Human-led content edited, structured, or drafted with AI assistance.
  3. `ai_generated`: Substantially drafted by AI with human review and editorial sign-off.
  4. `synthetic_media`: Deepfakes or synthetic realistic audio, imagery, or video.
- **Editorial Accountability:** Records human review completion and designated editorial responsibility holder (author, desk, or organization) for the revision.
- **Official Icons:** Bundles the European Commission AI content labels (basic AI, AI generated, AI modified) from https://digital-strategy.ec.europa.eu/en/policies/eu-icons-labelling-ai-generated-content.
- **Reader Display Options:** Configurable display modes:
  - *Reader Notice Card:* Formatted box with icon, explanation, and editorial oversight.
  - *Compact Badge:* Subtle inline pill badge.
  - *Icon Only:* Clean SVG icon with accessible tooltip.
  - *Layout Block:* Backdrop Layout block for placement in any layout region.
- **Machine Metadata:** Automatically injects standard `<meta>` tags and JSON-LD:
  - IPTC Digital Source Type (`http://cv.iptc.org/newscodes/digitalsourcetype/...`).
  - Schema.org JSON-LD provenance graph.
- **Central Compliance Report:** Audit dashboard at `admin/reports/ai-disclosure` with summary KPIs (total monitored, disclosure rate, human review rate, compliance gaps) and one-click streaming CSV export.
- **Developer API and hooks:** Other modules can call the API to record tool usage.

---

## Installation

1. Install AI Disclosure using the [Backdrop module installation guide](https://backdropcms.org/user-guide/modules). The module requires Backdrop CMS 1.x and PHP 8.2 or later.
2. Enable the module via **Administer > Modules** (`admin/modules`) or with Bee:
   ```bash
   bee pm-enable ai_disclosure
   ```
3. Open **Configuration → AI → AI Content Disclosure** (`admin/config/ai/ai-disclosure`) and select the content types to track.
4. Grant **Edit AI disclosure settings on content** to editors who should record disclosures. **Administer AI Disclosure** gives access to the settings, report, and CSV export.

---

## Configuration

Navigate to `admin/config/ai/ai-disclosure`:

- **Enabled content types:** Select which node bundles display disclosure tabs and badges.
- **Default disclosure level:** Choose default level for newly created content (`human_only`, `ai_assisted`, `ai_generated`, `synthetic_media`).
- **Default editorial responsibility:** Organization or desk responsible for editorial oversight (supports token `[site:name]`).
- **Display mode:** Notice Card, Compact Badge, Icon Only, or None.
- **Display position:** Top of body, bottom of body, or manual block-only.
- **Show badge for human-only:** Whether to display badges on content with zero AI assistance.
- **Show EU icons:** Toggle European Commission vector SVG disclosure icons.
- **Search engine metadata:** Toggle IPTC `digitalSourceType` meta tag and Schema.org JSON-LD injection.

## Record and verify a disclosure

1. Edit a page of an enabled content type and open **AI Disclosure**.
2. Choose the disclosure level. Select **Human editorial review completed** after someone reviews the content. Enter the desk or person responsible for the review.
3. Add optional public notes, such as “AI drafted the summary. An editor checked it against the source.” Keep private prompts and other confidential information out of the notes. The notice card and JSON-LD can show them to visitors.
4. Save the page. Open its full view and look for the configured notice. If display position is manual, place the **AI Content Disclosure** layout block on a layout used by the page.
5. Open the report at `admin/reports/ai-disclosure` to check the current revision and review status. Export CSV to download the filtered records.

---

## Layout Blocks & Tokens

### Layout Block
A dedicated block **AI Content Disclosure** (`ai_disclosure_node`) is available under **Administer > Structure > Layouts** (`admin/structure/layouts`). Use it to position the disclosure card in sidebars, headers, or footers without modifying node templates.

### Tokens
Available tokens under `node`:
- `[node:ai-disclosure-level]`: Machine name (`human_only`, `ai_assisted`, `ai_generated`, `synthetic_media`).
- `[node:ai-disclosure-title]`: Level title (e.g. `AI-Assisted`).
- `[node:ai-disclosure-badge]`: Rendered badge HTML with SVG icon.
- `[node:ai-disclosure-card]`: Rendered reader notice card HTML.
- `[node:ai-disclosure-icon]`: Rendered SVG vector icon markup.
- `[node:ai-disclosure-responsibility]`: Editorial accountability holder.
- `[node:ai-disclosure-notes]`: Transparency notes and prompts used.

---

## Developer API

### Loading Disclosure
```php
$record = ai_disclosure_get('node', $nid, $vid);
```

### Saving Disclosure
```php
ai_disclosure_save('node', $nid, $vid, [
  'disclosure_level' => 'ai_assisted',
  'human_review' => 1,
  'editorial_responsibility' => 'Science Desk',
  'notes' => 'Summary generated with Claude 3.5 Sonnet',
  'tool_used' => 'ai_content',
]);
```

### Recording Tool Usage from Generators
```php
ai_disclosure_record_tool_usage('node', $nid, 'ai_content', 'ai_assisted', $vid);
```

### Custom Taxonomy Levels
Implement `hook_ai_disclosure_levels_alter(&$levels)` to adjust labels or add custom levels:
```php
function mymodule_ai_disclosure_levels_alter(&$levels) {
  $levels['ai_assisted']['label'] = t('Co-Created with AI');
}
```

---

## License

This project is licensed under the GPL-2.0-or-later license.
The bundled European Commission icons are available under the Commission’s separate [icon licence](https://digital-strategy.ec.europa.eu/en/policies/eu-icons-labelling-ai-generated-content).
