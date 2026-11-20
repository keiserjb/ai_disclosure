<?php

/**
 * @file
 * Token integration for AI Disclosure.
 */

/**
 * Implements hook_token_info().
 */
function ai_disclosure_token_info() {
  $info = [];

  $info['tokens']['node']['ai-disclosure-level'] = [
    'name' => t('AI disclosure level machine name'),
    'description' => t('The machine name of the AI disclosure status (e.g. human_only, ai_assisted, ai_generated).'),
  ];
  $info['tokens']['node']['ai-disclosure-title'] = [
    'name' => t('AI disclosure level title'),
    'description' => t('The human-readable label of the AI disclosure level.'),
  ];
  $info['tokens']['node']['ai-disclosure-badge'] = [
    'name' => t('AI disclosure badge HTML'),
    'description' => t('The rendered compact badge HTML with icon.'),
  ];
  $info['tokens']['node']['ai-disclosure-card'] = [
    'name' => t('AI disclosure notice card HTML'),
    'description' => t('The rendered reader notice card HTML.'),
  ];
  $info['tokens']['node']['ai-disclosure-icon'] = [
    'name' => t('AI disclosure icon HTML'),
    'description' => t('The European Commission AI content label SVG for the disclosure level.'),
  ];
  $info['tokens']['node']['ai-disclosure-responsibility'] = [
    'name' => t('AI disclosure editorial responsibility'),
    'description' => t('The individual, organization, or desk taking editorial accountability.'),
  ];
  $info['tokens']['node']['ai-disclosure-notes'] = [
    'name' => t('AI disclosure transparency notes'),
    'description' => t('Explanatory notes, prompt techniques, and AI models utilized.'),
  ];

  return $info;
}

/**
 * Implements hook_tokens().
 */
function ai_disclosure_tokens($type, $tokens, array $data = [], array $options = []) {
  $replacements = [];
  $sanitize = !empty($options['sanitize']);

  if ($type === 'node' && !empty($data['node'])) {
    $node = $data['node'];
    $config = config('ai_disclosure.settings');
    $enabled = ai_disclosure_enabled_bundles();

    if (!in_array($node->type, $enabled)) {
      return $replacements;
    }

    $record = isset($node->ai_disclosure) ? (array) $node->ai_disclosure : ai_disclosure_get('node', $node->nid, !empty($node->vid) ? $node->vid : NULL);
    $levels = ai_disclosure_get_levels();
    $level_key = !empty($record['disclosure_level']) ? $record['disclosure_level'] : 'human_only';
    $level_info = isset($levels[$level_key]) ? $levels[$level_key] : $levels['human_only'];

    foreach ($tokens as $name => $original) {
      switch ($name) {
        case 'ai-disclosure-level':
          $replacements[$original] = $sanitize ? check_plain($level_key) : $level_key;
          break;

        case 'ai-disclosure-title':
          $replacements[$original] = $sanitize ? check_plain($level_info['title']) : $level_info['title'];
          break;

        case 'ai-disclosure-badge':
          $replacements[$original] = theme('ai_disclosure_badge', ['level' => $level_key, 'record' => $record, 'config' => $config]);
          break;

        case 'ai-disclosure-card':
          $replacements[$original] = theme('ai_disclosure_card', ['level' => $level_key, 'record' => $record, 'config' => $config]);
          break;

        case 'ai-disclosure-icon':
          $replacements[$original] = theme('ai_disclosure_icon', ['level' => $level_key, 'icon_name' => $level_info['icon']]);
          break;

        case 'ai-disclosure-responsibility':
          $val = ai_disclosure_responsibility($record);
          $replacements[$original] = $sanitize ? check_plain($val) : $val;
          break;

        case 'ai-disclosure-notes':
          $val = !empty($record['notes']) ? $record['notes'] : '';
          $replacements[$original] = $sanitize ? check_plain($val) : $val;
          break;
      }
    }
  }

  return $replacements;
}
