<?php

/**
 * @file
 * Administration settings form for AI Disclosure.
 */

/**
 * Form builder for the AI Disclosure configuration form.
 */
function ai_disclosure_settings_form(array $form, array &$form_state) {
  $config = config('ai_disclosure.settings');

  $form['#config'] = 'ai_disclosure.settings';

  $types = node_type_get_names();
  $form['enabled_bundles'] = [
    '#type' => 'checkboxes',
    '#title' => t('Enabled content types'),
    '#description' => t('Select which content types display AI disclosure tabs and badges.'),
    '#options' => $types,
    '#default_value' => $config->get('enabled_bundles') !== NULL ? $config->get('enabled_bundles') : ['article', 'page', 'post'],
  ];

  $levels = ai_disclosure_get_levels();
  $level_options = [];
  foreach ($levels as $key => $info) {
    $level_options[$key] = $info['title'] . ' — ' . $info['description'];
  }

  $form['default_level'] = [
    '#type' => 'select',
    '#title' => t('Default disclosure level'),
    '#description' => t('Default level applied when creating new content.'),
    '#options' => $level_options,
    '#default_value' => $config->get('default_level') ?: 'human_only',
  ];

  $form['default_editorial_responsibility'] = [
    '#type' => 'textfield',
    '#title' => t('Default editorial responsibility holder'),
    '#description' => t('Organization or desk responsible for editorial oversight (supports <code>[site:name]</code> token).'),
    '#default_value' => $config->get('default_editorial_responsibility') ?: '[site:name]',
  ];

  $form['display_settings'] = [
    '#type' => 'fieldset',
    '#title' => t('Display settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  ];

  $form['display_settings']['display_mode'] = [
    '#type' => 'radios',
    '#title' => t('Display mode'),
    '#options' => [
      'card' => t('Notice card (detailed box with icon, description, and editorial responsibility)'),
      'badge' => t('Compact badge (subtle inline pill badge)'),
      'icon' => t('Icon only (clean SVG icon with explanatory tooltip)'),
      'none' => t('None (automatic content display disabled; use Layout block)'),
    ],
    '#default_value' => $config->get('display_mode') ?: 'card',
  ];

  $form['display_settings']['display_position'] = [
    '#type' => 'radios',
    '#title' => t('Display position'),
    '#options' => [
      'top' => t('Top of content'),
      'bottom' => t('Bottom of content'),
      'manual' => t('Manual only (do not automatically inject into content body)'),
    ],
    '#default_value' => $config->get('display_position') ?: 'bottom',
  ];

  $form['display_settings']['show_human_only_badge'] = [
    '#type' => 'checkbox',
    '#title' => t('Show badge for human-only content'),
    '#description' => t('If unchecked, disclosure notices will only display for AI-assisted, AI-generated, or synthetic media content.'),
    '#default_value' => (bool) $config->get('show_human_only_badge'),
  ];

  $form['display_settings']['show_eu_icons'] = [
    '#type' => 'checkbox',
    '#title' => t('Show European Commission official disclosure icons'),
    '#description' => t('Displays standardized SVG icons published by the European Commission for AI transparency.'),
    '#default_value' => $config->get('show_eu_icons') !== NULL ? (bool) $config->get('show_eu_icons') : TRUE,
  ];

  $form['display_settings']['custom_disclaimer_text'] = [
    '#type' => 'textarea',
    '#title' => t('Custom disclaimer text'),
    '#description' => t('Optional supplemental note appended to disclosure notice cards.'),
    '#default_value' => $config->get('custom_disclaimer_text') ?: '',
    '#rows' => 3,
  ];

  $form['metadata_settings'] = [
    '#type' => 'fieldset',
    '#title' => t('Search engine and machine metadata'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  ];

  $form['metadata_settings']['enable_iptc_meta'] = [
    '#type' => 'checkbox',
    '#title' => t('Inject IPTC digitalSourceType meta tag'),
    '#description' => t('Emits <code>&lt;meta name="digitalSourceType" content="..."&gt;</code> into HTML &lt;head&gt; according to IPTC standards.'),
    '#default_value' => $config->get('enable_iptc_meta') !== NULL ? (bool) $config->get('enable_iptc_meta') : TRUE,
  ];

  $form['metadata_settings']['enable_schema_org_jsonld'] = [
    '#type' => 'checkbox',
    '#title' => t('Inject Schema.org JSON-LD provenance data'),
    '#description' => t('Emits a separate Schema.org CreativeWork node with additionalType set to the IPTC digital source URI. Does not alter the page Article graph.'),
    '#default_value' => $config->get('enable_schema_org_jsonld') !== NULL ? (bool) $config->get('enable_schema_org_jsonld') : TRUE,
  ];

  $form['#submit'][] = 'ai_disclosure_settings_form_clean_bundles';

  return system_settings_form($form);
}

/**
 * Submit handler to clean enabled bundle array keys.
 */
function ai_disclosure_settings_form_clean_bundles(array $form, array &$form_state) {
  if (isset($form_state['values']['enabled_bundles'])) {
    $form_state['values']['enabled_bundles'] = array_values(array_filter($form_state['values']['enabled_bundles']));
  }
}

/**
 * Page callback: AI Disclosure Compliance Report.
 */
function ai_disclosure_compliance_report_page() {
  backdrop_add_css(backdrop_get_path('module', 'ai_disclosure') . '/css/ai_disclosure.css');

  $config = config('ai_disclosure.settings');
  $enabled = ai_disclosure_enabled_bundles();

  if (empty($enabled)) {
    return [
      '#markup' => '<div class="messages warning">' . t('No content types are currently enabled for AI disclosure tracking. Enable content types on the <a href="!url">settings page</a>.', ['!url' => url('admin/config/ai/ai-disclosure')]) . '</div>',
    ];
  }

  // 1. KPI Metrics
  $total_nodes = (int) db_query("SELECT COUNT(*) FROM {node} WHERE type IN (:types)", [':types' => $enabled])->fetchField();

  $disclosed_nodes = (int) db_query("SELECT COUNT(*) FROM {node} n INNER JOIN {ai_disclosure} d ON d.entity_type = 'node' AND d.entity_id = n.nid AND d.revision_id = n.vid WHERE n.type IN (:types) AND d.disclosure_level != 'human_only'", [':types' => $enabled])->fetchField();

  $reviewed_ai_nodes = (int) db_query("SELECT COUNT(*) FROM {node} n INNER JOIN {ai_disclosure} d ON d.entity_type = 'node' AND d.entity_id = n.nid AND d.revision_id = n.vid WHERE n.type IN (:types) AND d.disclosure_level != 'human_only' AND d.human_review = 1", [':types' => $enabled])->fetchField();

  $compliance_gaps = (int) db_query("SELECT COUNT(*) FROM {node} n INNER JOIN {ai_disclosure} d ON d.entity_type = 'node' AND d.entity_id = n.nid AND d.revision_id = n.vid WHERE n.type IN (:types) AND d.disclosure_level IN ('ai_generated', 'synthetic_media') AND d.human_review = 0", [':types' => $enabled])->fetchField();

  $pct_disclosed = $total_nodes > 0 ? round(($disclosed_nodes / $total_nodes) * 100, 1) : 0;
  $pct_reviewed = $disclosed_nodes > 0 ? round(($reviewed_ai_nodes / $disclosed_nodes) * 100, 1) : 100;

  $kpi_html = '<div class="ai-disclosure-kpi-grid">';

  $kpi_html .= '<div class="ai-disclosure-kpi-card ai-disclosure-kpi-card--highlight">';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__label">' . t('Monitored Content') . '</div>';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__value">' . $total_nodes . '</div>';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__subtext">' . t('Nodes across @count enabled types', ['@count' => count($enabled)]) . '</div>';
  $kpi_html .= '</div>';

  $kpi_html .= '<div class="ai-disclosure-kpi-card ai-disclosure-kpi-card--highlight">';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__label">' . t('AI Disclosed') . '</div>';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__value">' . $disclosed_nodes . ' <span style="font-size: 1.1rem; font-weight: normal;">(' . $pct_disclosed . '%)</span></div>';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__subtext">' . t('Assisted, generated, or synthetic') . '</div>';
  $kpi_html .= '</div>';

  $kpi_html .= '<div class="ai-disclosure-kpi-card ' . ($pct_reviewed >= 90 ? 'ai-disclosure-kpi-card--success' : 'ai-disclosure-kpi-card--warning') . '">';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__label">' . t('Editorial Sign-off') . '</div>';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__value">' . $pct_reviewed . '%</div>';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__subtext">' . t('@reviewed of @total disclosed reviewed', ['@reviewed' => $reviewed_ai_nodes, '@total' => $disclosed_nodes]) . '</div>';
  $kpi_html .= '</div>';

  $kpi_html .= '<div class="ai-disclosure-kpi-card ' . ($compliance_gaps > 0 ? 'ai-disclosure-kpi-card--danger' : 'ai-disclosure-kpi-card--success') . '">';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__label">' . t('Compliance Gaps') . '</div>';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__value">' . $compliance_gaps . '</div>';
  $kpi_html .= '<div class="ai-disclosure-kpi-card__subtext">' . ($compliance_gaps > 0 ? t('Unreviewed AI or synthetic media') : t('All synthetic media verified')) . '</div>';
  $kpi_html .= '</div>';

  $kpi_html .= '</div>';

  // 2. Export button & Filter form
  $export_params = $_GET;
  unset($export_params['q']);
  $export_url = url('admin/reports/ai-disclosure/export', ['query' => $export_params]);

  $actions_html = '<div class="ai-disclosure-report-actions">';
  $actions_html .= '<div><h3>' . t('Audit Registry') . '</h3></div>';
  $actions_html .= '<div><a href="' . check_url($export_url) . '" class="button button-secondary">📥 ' . t('Export CSV') . '</a></div>';
  $actions_html .= '</div>';

  $filter_form = backdrop_get_form('ai_disclosure_report_filter_form', $enabled);

  // 3. Table of entities
  $header = [
    ['data' => t('Title'), 'field' => 'n.title'],
    ['data' => t('Type'), 'field' => 'n.type'],
    ['data' => t('Disclosure Level'), 'field' => 'd.disclosure_level'],
    ['data' => t('Review'), 'field' => 'd.human_review'],
    ['data' => t('Editorial Accountability'), 'field' => 'd.editorial_responsibility'],
    ['data' => t('Tools Used'), 'field' => 'd.tool_used'],
    ['data' => t('Last Updated'), 'field' => 'n.changed', 'sort' => 'desc'],
    ['data' => t('Operations')],
  ];

  $query = db_select('node', 'n')->extend('PagerDefault')->extend('TableSort');
  $query->leftJoin('ai_disclosure', 'd', 'd.entity_type = :type AND d.entity_id = n.nid AND d.revision_id = n.vid', [':type' => 'node']);
  $query->fields('n', ['nid', 'vid', 'title', 'type', 'status', 'created', 'changed', 'uid']);
  $query->fields('d', ['disclosure_level', 'human_review', 'editorial_responsibility', 'notes', 'tool_used']);

  // Apply filters
  $type_filter = !empty($_GET['bundle']) ? $_GET['bundle'] : '';
  if ($type_filter && in_array($type_filter, $enabled)) {
    $query->condition('n.type', $type_filter);
  }
  else {
    $query->condition('n.type', $enabled, 'IN');
  }

  $level_filter = !empty($_GET['level']) ? $_GET['level'] : '';
  if ($level_filter) {
    if ($level_filter === 'human_only') {
      $query->condition(db_or()
        ->condition('d.disclosure_level', 'human_only')
        ->isNull('d.disclosure_level')
      );
    }
    else {
      $query->condition('d.disclosure_level', $level_filter);
    }
  }

  $review_filter = isset($_GET['review']) && $_GET['review'] !== '' ? $_GET['review'] : '';
  if ($review_filter === '1') {
    $query->condition('d.human_review', 1);
  }
  elseif ($review_filter === '0') {
    $query->condition(db_or()
      ->condition('d.human_review', 0)
      ->isNull('d.human_review')
    );
  }

  $search_filter = !empty($_GET['search']) ? trim($_GET['search']) : '';
  if ($search_filter) {
    $query->condition(db_or()
      ->condition('n.title', '%' . db_like($search_filter) . '%', 'LIKE')
      ->condition('d.editorial_responsibility', '%' . db_like($search_filter) . '%', 'LIKE')
      ->condition('d.notes', '%' . db_like($search_filter) . '%', 'LIKE')
    );
  }

  $query->limit(25)->orderByHeader($header);
  $results = $query->execute()->fetchAll();
  $nodes = node_load_multiple(array_column($results, 'nid'));

  $types_info = node_type_get_names();
  $levels = ai_disclosure_get_levels();

  $rows = [];
  foreach ($results as $row) {
    $level_key = !empty($row->disclosure_level) ? $row->disclosure_level : ($config->get('default_level') ?: 'human_only');
    $badge_html = theme('ai_disclosure_badge', [
      'level' => $level_key,
      'record' => (array) $row,
      'config' => $config,
    ]);

    $resp = !empty($row->editorial_responsibility) ? $row->editorial_responsibility : token_replace($config->get('default_editorial_responsibility') ?: '[site:name]');

    $review_label = ($row->human_review !== NULL && (int) $row->human_review === 1) ? '<span style="color:#10b981;">✓ ' . t('Yes') . '</span>' : '<span style="color:#ef4444;">✗ ' . t('No') . '</span>';

    $ops = [];
    $ops[] = l(t('View'), 'node/' . $row->nid);
    if (isset($nodes[$row->nid]) && node_access('update', $nodes[$row->nid])) {
      $ops[] = l(t('Edit'), 'node/' . $row->nid . '/edit', ['query' => backdrop_get_destination()]);
    }

    $rows[] = [
      l($row->title ?: t('(Untitled)'), 'node/' . $row->nid),
      check_plain(isset($types_info[$row->type]) ? $types_info[$row->type] : $row->type),
      $badge_html,
      $review_label,
      check_plain($resp),
      check_plain($row->tool_used ?: '-'),
      format_date($row->changed, 'short'),
      implode(' | ', $ops),
    ];
  }

  $table_html = theme('table', [
    'header' => $header,
    'rows' => $rows,
    'empty' => t('No content matches the selected AI disclosure filters.'),
  ]);
  $pager_html = theme('pager');

  return [
    'kpi' => ['#markup' => $kpi_html],
    'actions' => ['#markup' => $actions_html],
    'filter' => $filter_form,
    'table' => ['#markup' => $table_html],
    'pager' => ['#markup' => $pager_html],
  ];
}

/**
 * Filter form builder for AI disclosure compliance report.
 */
function ai_disclosure_report_filter_form(array $form, array &$form_state, array $enabled_bundles) {
  $types = node_type_get_names();
  $bundle_options = ['' => t('- All enabled types -')];
  foreach ($enabled_bundles as $type) {
    if (isset($types[$type])) {
      $bundle_options[$type] = $types[$type];
    }
  }

  $levels = ai_disclosure_get_levels();
  $level_options = ['' => t('- All disclosure levels -')];
  foreach ($levels as $key => $info) {
    $level_options[$key] = $info['title'];
  }

  $form = [
    '#method' => 'get',
    '#attributes' => ['class' => ['container-inline', 'ai-disclosure-filter-form']],
  ];

  $form['bundle'] = [
    '#type' => 'select',
    '#title' => t('Type'),
    '#options' => $bundle_options,
    '#default_value' => isset($_GET['bundle']) ? $_GET['bundle'] : '',
  ];

  $form['level'] = [
    '#type' => 'select',
    '#title' => t('Level'),
    '#options' => $level_options,
    '#default_value' => isset($_GET['level']) ? $_GET['level'] : '',
  ];

  $form['review'] = [
    '#type' => 'select',
    '#title' => t('Review'),
    '#options' => [
      '' => t('- All review statuses -'),
      '1' => t('Human reviewed'),
      '0' => t('Not reviewed'),
    ],
    '#default_value' => isset($_GET['review']) ? $_GET['review'] : '',
  ];

  $form['search'] = [
    '#type' => 'textfield',
    '#title' => t('Search'),
    '#default_value' => isset($_GET['search']) ? $_GET['search'] : '',
    '#size' => 20,
    '#attributes' => ['placeholder' => t('Title, editor, notes...')],
  ];

  $form['actions'] = [
    '#type' => 'actions',
  ];

  $form['actions']['submit'] = [
    '#type' => 'submit',
    '#value' => t('Filter'),
  ];

  if (!empty($_GET['bundle']) || !empty($_GET['level']) || (isset($_GET['review']) && $_GET['review'] !== '') || !empty($_GET['search'])) {
    $form['actions']['reset'] = [
      '#type' => 'markup',
      '#markup' => l(t('Reset'), 'admin/reports/ai-disclosure', ['attributes' => ['class' => ['button', 'button-secondary']]]),
    ];
  }

  return $form;
}

/**
 * Streams a CSV export of the AI disclosure compliance dataset.
 */
function ai_disclosure_export_csv() {
  $config = config('ai_disclosure.settings');
  $enabled = ai_disclosure_enabled_bundles();
  if (empty($enabled)) {
    backdrop_goto('admin/reports/ai-disclosure');
  }

  $query = db_select('node', 'n');
  $query->leftJoin('ai_disclosure', 'd', 'd.entity_type = :type AND d.entity_id = n.nid AND d.revision_id = n.vid', [':type' => 'node']);
  $query->leftJoin('users', 'u', 'u.uid = n.uid');
  $query->fields('n', ['nid', 'vid', 'title', 'type', 'status', 'created', 'changed']);
  $query->fields('d', ['disclosure_level', 'human_review', 'editorial_responsibility', 'notes', 'tool_used']);
  $query->addField('u', 'name', 'author_name');

  // Apply active filters
  $type_filter = !empty($_GET['bundle']) ? $_GET['bundle'] : '';
  if ($type_filter && in_array($type_filter, $enabled)) {
    $query->condition('n.type', $type_filter);
  }
  else {
    $query->condition('n.type', $enabled, 'IN');
  }

  $level_filter = !empty($_GET['level']) ? $_GET['level'] : '';
  if ($level_filter) {
    if ($level_filter === 'human_only') {
      $query->condition(db_or()
        ->condition('d.disclosure_level', 'human_only')
        ->isNull('d.disclosure_level')
      );
    }
    else {
      $query->condition('d.disclosure_level', $level_filter);
    }
  }

  $review_filter = isset($_GET['review']) && $_GET['review'] !== '' ? $_GET['review'] : '';
  if ($review_filter === '1') {
    $query->condition('d.human_review', 1);
  }
  elseif ($review_filter === '0') {
    $query->condition(db_or()
      ->condition('d.human_review', 0)
      ->isNull('d.human_review')
    );
  }

  $search_filter = !empty($_GET['search']) ? trim($_GET['search']) : '';
  if ($search_filter) {
    $query->condition(db_or()
      ->condition('n.title', '%' . db_like($search_filter) . '%', 'LIKE')
      ->condition('d.editorial_responsibility', '%' . db_like($search_filter) . '%', 'LIKE')
      ->condition('d.notes', '%' . db_like($search_filter) . '%', 'LIKE')
    );
  }

  $query->orderBy('n.changed', 'DESC');
  $results = $query->execute();

  $filename = 'ai-disclosure-compliance-' . date('Y-m-d') . '.csv';

  backdrop_add_http_header('Content-Type', 'text/csv; charset=utf-8');
  backdrop_add_http_header('Content-Disposition', 'attachment; filename="' . $filename . '"');
  backdrop_add_http_header('Pragma', 'no-cache');
  backdrop_add_http_header('Expires', '0');

  $output = fopen('php://output', 'w');

  // CSV column headers
  fputcsv($output, [
    'Node ID',
    'Revision ID',
    'Title',
    'Content Type',
    'Published Status',
    'Disclosure Level',
    'Human Review Completed',
    'Editorial Responsibility Holder',
    'AI Tools Used',
    'Transparency Notes',
    'Author',
    'Created',
    'Last Changed',
    'Content URL',
  ], ',', '"', '');

  $levels = ai_disclosure_get_levels();

  foreach ($results as $row) {
    $level_key = !empty($row->disclosure_level) ? $row->disclosure_level : ($config->get('default_level') ?: 'human_only');
    $level_title = isset($levels[$level_key]) ? $levels[$level_key]['title'] : $level_key;

    $resp = !empty($row->editorial_responsibility) ? $row->editorial_responsibility : token_replace($config->get('default_editorial_responsibility') ?: '[site:name]');

    $reviewed = ($row->human_review !== NULL && (int) $row->human_review === 1) ? 'Yes' : 'No';

    $columns = [
      $row->nid,
      $row->vid,
      $row->title,
      $row->type,
      $row->status ? 'Published' : 'Draft',
      $level_title . ' (' . $level_key . ')',
      $reviewed,
      $resp,
      $row->tool_used ?: '',
      $row->notes ?: '',
      $row->author_name ?: 'Anonymous',
      date('Y-m-d H:i:s', $row->created),
      date('Y-m-d H:i:s', $row->changed),
      url('node/' . $row->nid, ['absolute' => TRUE]),
    ];
    // Keep editor-supplied text from being interpreted as spreadsheet formulas.
    foreach ($columns as &$value) {
      if (is_string($value) && preg_match('/^(?:[\t\r\n]|\s*[=+@-])/', $value)) {
        $value = "'" . $value;
      }
    }
    unset($value);
    fputcsv($output, $columns, ',', '"', '');
  }

  fclose($output);
  backdrop_exit();
}
