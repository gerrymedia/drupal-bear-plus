<?php

/**
 * @file
 * Administration UI callbacks for the advagg module.
 */
 
function _advagg_form_system_performance_settings_alter(&$form, &$form_state) {
  $advagg_enabled = isset($form_state['values']['advagg_enabled']) ? (int) $form_state['values']['advagg_enabled'] : (int) variable_get('advagg_enabled', ADVAGG_ENABLED);
  // Provide wrapper so AJAX can replace it.
  $form['bandwidth_optimization']['#prefix'] = '<div id="bandwidth-optimization">';
  $form['bandwidth_optimization']['#suffix'] = '</div>';
  $form['bandwidth_optimization']['aggregation'] = array(
    '#type' => 'fieldset',
    '#title' => t('CSS/JS Aggregation'),
    '#tree' => FALSE,
  );
  // Create toggle for enabling AdvAgg.
  $form['bandwidth_optimization']['aggregation']['advagg_enabled'] = array(
    '#type' => 'radios',
    '#weight' => -1,
    '#options' => array(
      0 => t('Default'),
      1 => t('Advanced'),
    ),
    '#default_value' => $advagg_enabled,
    '#ajax' => array(
      'callback' => 'advagg_form_system_performance_settings_ajax',
      'wrapper' => 'bandwidth-optimization',
      'method' => 'replace',
    ),
  );
  // Create vertical tabs for settings.
  $form['bandwidth_optimization']['aggregation']['aggregation_tabs'] = array(
    '#type' => 'vertical_tabs',
    '#tree' => FALSE,
  );
  // Create fieldset for core settings.
  $form['bandwidth_optimization']['aggregation']['core'] = array(
    '#access' => !(bool)$advagg_enabled,
    '#type' => 'fieldset',
    '#title' => t('Settings'),
    '#group' => 'aggregation_tabs',
    '#tree' => FALSE,
  );
  // Move core settings into fieldset.
  $form['bandwidth_optimization']['aggregation']['core']['preprocess_css'] = $form['bandwidth_optimization']['preprocess_css'];
  $form['bandwidth_optimization']['aggregation']['core']['preprocess_js'] = $form['bandwidth_optimization']['preprocess_js'];
  unset($form['bandwidth_optimization']['preprocess_css']);
  unset($form['bandwidth_optimization']['preprocess_js']);
  // If AdvAgg is not selected, then do not continue altering the form.
  if (!$advagg_enabled) {
    return;
  }
  // Add AdvAgg settings to form.
  $form['bandwidth_optimization']['aggregation']['advagg'] = array(
    '#type' => 'container',
    '#tree' => FALSE,
  );
  // Allow all modules to implement AdvAgg settings.
  foreach (module_implements('advagg_admin_form') as $module) {
    $function = $module . '_advagg_admin_form';
    $function($form['bandwidth_optimization']['aggregation']['advagg'], $form_state);
  }
  // Ensure fieldsets are assigned to the vertical tabs group and values are
  // not inside a tree.
  foreach (element_children($form['bandwidth_optimization']['aggregation']['advagg']) as $key) {
    if (isset($form['bandwidth_optimization']['aggregation']['advagg'][$key]['#type']) && $form['bandwidth_optimization']['aggregation']['advagg'][$key]['#type'] == 'fieldset') {
      $form['bandwidth_optimization']['aggregation']['advagg'][$key]['#group'] = 'aggregation_tabs';
      $form['bandwidth_optimization']['aggregation']['advagg'][$key]['#tree'] = FALSE;
    }
  }
}

function advagg_form_system_performance_settings_ajax($form, $form_state) {
  return $form['bandwidth_optimization'];
}

/**
 * Implements hook_advagg_admin_form().
 * @see: advagg_form_system_performance_settings_alter().
 */
function advagg_advagg_admin_form(&$form, &$form_state) {
  
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General'),
    '#weight' => -100,
  );
  _advagg_admin_form_general($form['general'], $form_state);

  $form['development'] = array(
    '#type' => 'fieldset',
    '#title' => t('Development'),
    '#weight' => 100,
  );
  _advagg_admin_settings_cache($form['development'], $form_state);
  
  $form['info'] = array(
    '#type' => 'fieldset',
    '#title' => t('Information'),
    '#weight' => 101,
  );
  _advagg_admin_settings_information($form['info'], $form_state);

}

function _advagg_admin_form_general(&$form, &$form_state) {
  $file_path = drupal_get_path('module', 'advagg');
  drupal_add_js(array('advagg' => array('key' => md5(drupal_get_private_key()))), 'setting');
  drupal_add_js($file_path . '/advagg.admin.js');
  
  $readme = $file_path . '/README.txt';
  
  $advagg_enabled = isset($form_state['values']['advagg_enabled']) ? (int) $form_state['values']['advagg_enabled'] : (int) variable_get('advagg_enabled', ADVAGG_ENABLED);
    
  $form['advagg_closure'] = array(
    '#type' => 'checkbox',
    '#title' => t('Aggregate footer JavaScript'),
    '#default_value' => variable_get('advagg_closure', ADVAGG_CLOSURE),
    '#description' => t('If enabled javascript files in the closure region will be aggregated.'),
  );
  // Make sure the advagg_check_missing_handler function is available.
  module_load_install('advagg');
  $ret = advagg_check_missing_handler();
  $form['advagg_async_generation'] = array(
    '#type' => 'checkbox',
    '#title' => t('Generate CSS/JS files on request (async mode)'),
    '#default_value' => variable_get('advagg_async_generation', ADVAGG_ASYNC_GENERATION),
    '#disabled' => $ret['advagg_async_generation']['severity'] == REQUIREMENT_ERROR ? TRUE : FALSE,
    '#description' => t('Current State: !value', array(
      '!value' => filter_xss(' '
        . $ret['advagg_async_generation']['value'] . ' '
        . (!empty($ret['advagg_async_generation']['description']) ? $ret['advagg_async_generation']['description'] : '') . ' '
      ),
    )),
  );
  $form['advagg_gzip_compression'] = array(
    '#type' => 'checkbox',
    '#title' => t('Gzip CSS/JS files'),
    '#default_value' => variable_get('advagg_gzip_compression', ADVAGG_GZIP_COMPRESSION),
    '#description' => t('This might break CSS/JS handling at the Apache level. If it does, use the rules for your webroot level htaccess file before re-enabling. Directions on what to change are located in the <a href="@readme">readme</a> file. In short, be sure to test this out.', array('@readme' => url($readme))),
  );
  $form['advagg_dir_htaccess'] = array(
    '#type' => 'checkbox',
    '#title' => t('Generate .htaccess files in the advagg_* dirs'),
    '#default_value' => variable_get('advagg_dir_htaccess', ADVAGG_DIR_HTACCESS),
    '#description' => t('Disable if your using the rules from the <a href="@readme">readme</a> file in your webroot level htaccess file.', array('@readme' => url($readme))),
  );
  $form['advagg_rebuild_on_flush'] = array(
    '#type' => 'checkbox',
    '#title' => t('Regenerate flushed bundles in the cache flush request'),
    '#default_value' => variable_get('advagg_rebuild_on_flush', ADVAGG_REBUILD_ON_FLUSH),
    '#description' => t('You can enable if your server will not timeout on a request. This will call advagg_rebuild_bundle() as a <a href="http://php.net/register-shutdown-function">shutdown function</a> for every bundle that has been marked as expired; thus rebuilding that bundle in the same request as the flush.'),
  );
  if (file_default_scheme() !== 'public') {
    $extra = module_exists('boost') ? t('If boost is installed, you can use the cache directory for advagg files.') : '';
    $form['advagg_custom_files_dir'] = array(
      '#type' => 'textfield',
      '#field_prefix' => '<div>' . t('You are using a private file system. You must serve aggregated files via a public folder.') . '</div>',
      '#title' => t('Use a different directory for storing advagg files'),
      '#default_value' => variable_get('advagg_custom_files_dir', ADVAGG_CUSTOM_FILES_DIR),
      '#description' => check_plain(t('You need to create a publicly writable directory (same permissions as Drupals files folder).') . ' ' . $extra . ' ' . t('If you do not need this, you can put a space in this box.')),
      '#required' => TRUE,
    );
  }
  $options = array(
    0 => t('Wait for locks'),
    1 => t('Do not wait for locks'),
    2 => t('Only serve aggregated files if they are already built (only works if async is enabled)'),
  );
  $form['advagg_aggregate_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Aggregation Inclusion Mode'),
    '#default_value' => variable_get('advagg_aggregate_mode', ADVAGG_AGGREGATE_MODE),
    '#options' => $options,
    '#description' => t('Should the page wait for the aggregate to be built before including the file, or should it send out the page with aggregates not included.'),
  );
  $form['advagg_page_cache_mode'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disable page caching if all aggregates are not included on the page.'),
    '#default_value' => variable_get('advagg_page_cache_mode', ADVAGG_PAGE_CACHE_MODE),
  );
  $form['advagg_checksum_mode'] = array(
    '#type' => 'radios',
    '#title' => t('File Checksum Mode'),
    '#default_value' => variable_get('advagg_checksum_mode', ADVAGG_CHECKSUM_MODE),
    '#options' => array(
      'filemtime' => function_exists('filemtime') ? 'filemtime' : 'filemtime (not installed)',
      'md5' => function_exists('md5') ? 'md5' : 'md5 (not installed)',
    ),
    '#description' => t('If Drupal is on multiple servers and the file system is not shared;  using md5 is recommended. The file modification time (filemtime) could be different with this type of setup. WARNING: Changing this value will invalidate all bundles and new ones will have to be built.'),
  );
  $form['advagg_server_addr'] = array(
    '#type' => 'textfield',
    '#title' => t('IP Address to send all asynchronous requests to'),
    '#default_value' => variable_get('advagg_server_addr', FALSE),
    '#description' => t('If left blank it will use the same server as the request. If set to -1 it will use the host name instead of an IP address.'),
  );
//   $form['advagg_debug'] = array(
//     '#type'           => 'checkbox',
//     '#title'          => t('Debug to watchdog.'),
//     '#default_value'  => variable_get('advagg_debug', ADVAGG_DEBUG),
//   );
}

function _advagg_admin_settings_cache(&$form, &$form_state) {
  $bundle_count = db_query("SELECT COUNT(*) FROM (SELECT bundle_md5 FROM {advagg_bundles} GROUP BY bundle_md5) as temp")->fetchField();
  $callbacks = array(
    'advagg_admin_flush_cache' => array(
      '#value' => t('Flush AdvAgg Cache'),
      '#description' => t('Scan all files referenced in aggregated files. If any of them have changed, increment the counters containing that file and rebuild the bundle.'),
    ),
    'advagg_admin_batch_rebuild' => array(
      '#value' => t('Increment Counters'),
      '#description' => t('Recreate all aggregated files by incrementing internal counter for every bundle. %count Files', array('%count' => $bundle_count)),
    ),
    'advagg_admin_master_reset' => array(
      '#value' => t('Master Reset'),
      '#description' => t('Clean Slate - Truncate all advagg tables and delete all advagg files. Useful for testing purposes. Running this on a production site is not a good idea.'),
    ),
    'advagg_admin_recreate_htaccess' => array(
      '#value' => t('Recreate htaccess files'),
      '#description' => t('This will recreate the htaccess files located in the advagg_* directories.'),
    ),
    'advagg_admin_toggle_bypass_cookie' => array(
      '#value' => t('Toggle By-Pass Cookie'),
      '#description' => t('This will set or remove a cookie that disables aggregation for the remainder of this browser session.'),
    ),
  );
  // Generate dynamic buttons.
  foreach ($callbacks as $callback => $data) {
    $form[$callback] = array(
      '#type' => 'submit',
      '#value' => $data['#value'],
      '#prefix' => '<div style="clear: both; float: left; margin-bottom: 20px; width: 100%;">',
      '#suffix' => '<div class="description" style="margin-bottom: 20px; overflow: auto;">' . $data['#description'] . '</div></div>',
      '#attributes' => array(
        'style' => 'float: left; margin-right: 10px; min-width: 200px;',
      ),
      '#submit' => array($callback),
    );
  }  
}

function _advagg_admin_settings_information(&$form, &$form_state) {
  $form['hook_process_html'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#title' => t('$theme_registry[html][process functions]'),
    '#description' => t('Ensure that %advagg_process_html is at the bottom of the array of functions. It is vital for this function to be the last in the list as it will be altering and completely replacing the CSS and JavaScript loaded on each page request. If other functions appear after it, you may experience undesirable functionality or this module may not work at all.', array('%advagg_process_html' => 'advagg_process_html')),
  );
  drupal_theme_initialize();
  $theme_registry = theme_get_registry();
  if (module_exists('devel')) {
    $form['hook_process_html']['value'] = array(
      '#markup' => kprint_r($theme_registry['html']['process functions'], TRUE),
    );
  }
  // Otherwise, throw output in a textarea.
  else {
    $form['hook_process_html']['value'] = array(
      '#type' => 'textarea',
      '#default_value' => implode("\n", $hooks['html']['process functions']),
      '#rows' => count($theme_registry['html']['process functions']) + 1,
    );
  }

  $types = db_query("SELECT DISTINCT(filetype) FROM {advagg_files}");
  while ($type = $types->fetchField()) {
    $form[$type] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#title' => t('@type files', array('@type' => drupal_strtoupper($type))),
    );
    // filename  filename_md5  checksum  filetype  counter   data
    $results = db_query("SELECT * FROM {advagg_files} WHERE filetype = :filetype", array(':filetype' => $type));
    while ($row = db_fetch_array($results)) {
      $data = advagg_get_file_data($row['filename_md5']);
      if (!empty($data)) {
        list($data, $rows) = advagg_form_print_r($data);

        $form[$type][$row['filename_md5']] = array(
          '#type' => 'textarea',
          '#title' => check_plain($row['filename']),
          '#default_value' => $data,
          '#rows' => $rows -1,
          '#description' => t('File has changed %counter times', array('%counter' => $row['counter'])),
        );
      }
      else {
        $form[$type][$row['filename_md5']] = array(
          '#markup' => '<div>' . format_plural($row['counter'], 'changed 1 time - %file<br />', 'changed %counter times - %file<br /></div>',    array(
            '%counter' => $row['counter'],
            '%file' => $row['filename'],
          )),
        );
      }
    }
  }

  // Get hooks in use.
  $hooks = array(
    'advagg_admin_form' => array(),
    'advagg_css_alter' => array(),
    'advagg_css_inline_alter' => array(),
    'advagg_css_pre_alter' => array(),
    'advagg_css_extra_alter' => array(),
    'advagg_js_alter' => array(),
    'advagg_js_inline_alter' => array(),
    'advagg_js_pre_alter' => array(),
    'advagg_js_extra_alter' => array(),
    'advagg_js_header_footer_alter' => array(),
    'advagg_filenames_alter' => array(),
    'advagg_files_table' => array(),
    'advagg_master_reset' => array(),
    'advagg_disable_processor' => array(),
    'advagg_disable_page_cache' => array(),
    'advagg_bundler_analysis_alter' => array(),
  );
  foreach ($hooks as $hook => $values) {
    $hooks[$hook] = module_implements($hook);
  }
  // Record function overrides. Not working currently.
//   $hooks['advagg_css_render_function'] = array(variable_get('advagg_css_render_function', ADVAGG_CSS_RENDER_FUNCTION));
//   $hooks['advagg_js_render_function'] = array(variable_get('advagg_js_render_function', ADVAGG_JS_RENDER_FUNCTION));
//   $hooks['advagg_file_save_function'] = array(variable_get('advagg_file_save_function', ADVAGG_FILE_SAVE_FUNCTION));

  $form['hooks_implemented'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => t('Modules implementing advagg hooks'),
  );
  // Output hooks in form.
  foreach ($hooks as $hook => $values) {
    if (empty($values)) {
      $form['hooks_implemented'][$hook] = array(
        '#markup' => '<div><strong>' . check_plain($hook) . ':</strong><br />&nbsp;&nbsp;&nbsp;&nbsp;' . t('None') . '</div>',
      );
    }
    else {
      $form['hooks_implemented'][$hook] = array(
        '#markup' => '<div><strong>' . check_plain($hook) . ':</strong><br />&nbsp;&nbsp;&nbsp;&nbsp;' . filter_xss(implode('<br />&nbsp;&nbsp;&nbsp;&nbsp;', $values)) . '</div>',
      );
    }
  }

  // If any files are missing list them.
  // TODO: Reset missing files list.
  $results = db_query("SELECT * FROM {advagg_files} WHERE checksum = :checksum ORDER BY filetype ASC", array(':checksum' => '-1'));
  while ($row = $results->fetchAssoc()) {
    $form['missing'][$row['filename_md5']] = array(
      '#type' => 'markup',
      '#value' => '<div>' . check_plain($row['filename']) . '<br /></div>',
    );
  }
  if (!empty($form['missing'])) {
    $form['missing'] += array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#title' => t('Missing files'),
    );
  }

  // Asynchronous raw output
  $form['async'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => t('Asynchronous debug info'),
  );

  list($css_path, $js_path) = advagg_get_root_files_dir();

  $filepath = $css_path . '/css_missing' . mt_rand() . REQUEST_TIME . '_0.css';
  $url = _advagg_build_url($filepath);
  $headers = array(
    'Host' => $_SERVER['HTTP_HOST'],
    'Connection' => 'close',
  );

  timer_start(__FUNCTION__ . 'local');
  $data_local = drupal_http_request($url, array('headers' => $headers));
  $data_local->timer = timer_stop(__FUNCTION__ . 'local');
  // Use Krumo if Devel is installed.
  if (module_exists('devel')) {
    $form['async']['cdn'] = array(
      '#markup' => kprint_r($data_local, TRUE, t('Local Asynchronous Mode') . ' (' . check_plain($url) . ')'),
    );
  }
  // Otherwise, throw output in a textarea.
  else {
    list($data, $rows) = advagg_form_print_r($data_local);
    $form['async']['normal'] = array(
      '#type' => 'textarea',
      '#title' => t('Local Asynchronous Mode') . ' (' . check_plain($url) . ')',
      '#default_value' => $data,
      '#rows' => min($rows + 1, 50),
    );
  }

  if (module_exists('cdn')) {
    global $conf;
    $path_blacklist = variable_get(CDN_EXCEPTION_DRUPAL_PATH_BLACKLIST_VARIABLE, CDN_EXCEPTION_DRUPAL_PATH_BLACKLIST_DEFAULT);
    $conf[CDN_EXCEPTION_DRUPAL_PATH_BLACKLIST_VARIABLE] = '';

    $url_cdn = advagg_build_uri($filepath);

    $conf[CDN_EXCEPTION_DRUPAL_PATH_BLACKLIST_VARIABLE] = $path_blacklist;
    // Send request and also time it.
    timer_start(__FUNCTION__ . 'cdn');
    $data_cdn = drupal_http_request($url_cdn);
    $data_cdn->timer = timer_stop(__FUNCTION__ . 'cdn');
    // Use Krumo if Devel is installed.
    if (module_exists('devel')) {
      $form['async']['cdn'] = array(
        '#markup' => kprint_r($data_cdn, TRUE, t('CDN Asynchronous Mode') . ' (' . check_plain($url) . ')'),
      );
    }
    // Otherwise, throw output in a textarea.
    else {
      list($data, $rows) = advagg_form_print_r($data_cdn);
      $form['async']['cdn'] = array(
        '#type' => 'textarea',
        '#title' => t('CDN Asynchronous Mode') . ' (' . check_plain($url) . ')',
        '#default_value' => $data,
        '#rows' => min($rows + 1, 50),
      );
    }
  }
}

/**
 * Validate form values. Used to unset variables before they get saved.
 */
function advagg_admin_settings_form_validate($form, &$form_state) {
  global $conf;

  // Custom directory handling.
  if (!empty($form_state['values']['advagg_custom_files_dir'])) {
    $form_state['values']['advagg_custom_files_dir'] = trim($form_state['values']['advagg_custom_files_dir']);
    $files_dir = $form_state['values']['advagg_custom_files_dir'];
    if (!empty($files_dir) && $files_dir != $conf['advagg_custom_files_dir']) {

      // Try to create dir structure.
      $cumulative = '';
      $path_array = explode('/', $files_dir);
      foreach ($path_array as $dir) {
        if (empty($cumulative)) {
          $cumulative = $dir;
        }
        else {
          $cumulative .= '/' . $dir;
        }
        // If dir creation fails, bail out of loop.
        if (!file_prepare_directory($cumulative, FILE_CREATE_DIRECTORY)) {
          break;
        }
      }

      // See if dir exists and is writable.
      if (file_prepare_directory($files_dir) == TRUE) {

        // Remove old files.
        list($css_path, $js_path) = advagg_get_root_files_dir();
        file_scan_directory($css_path, '/.*/', array('callback' => 'file_unmanaged_delete'));
        @unlink($css_path);
        file_scan_directory($js_path, '/.*/', array('callback' => 'file_unmanaged_delete'));
        @unlink($js_path);

        // Set new path.
        $conf['advagg_custom_files_dir'] = $files_dir;
        advagg_get_root_files_dir(TRUE);
        menu_rebuild();
      }
      else {
        form_set_error('advagg_custom_files_dir', t('%dir is not a directory or is not writable by the web server.', array('%dir' => $files_dir)));
      }
    }
  }

  // If the IP field is not blank, check that its a valid address.
  if (!empty($form_state['values']['advagg_server_addr']) && $form_state['values']['advagg_server_addr'] != -1 && ip2long($form_state['values']['advagg_server_addr']) === FALSE) {
    form_set_error('advagg_server_addr', t('Must be a valid IP address.'));
  }

  // Remove non variable form info.
  unset($form_state['values']['advagg_flush']);
  unset($form_state['values']['advagg_rebuild']);
  unset($form_state['values']['advagg_forced_build']);
  unset($form_state['values']['advagg_recreate_htaccess']);
  unset($form_state['values']['advagg_reset']);
}

/**
 * Validate form values. Used to unset variables before they get saved.
 */
function advagg_admin_settings_form_submit($form, &$form_state) {
  global $conf;

  // Gzip & htaccess checks.
  list($css_path, $js_path) = advagg_get_root_files_dir();
  $css_path .= '/.htaccess';
  $js_path .= '/.htaccess';

  if ($conf['advagg_gzip_compression'] != $form_state['values']['advagg_gzip_compression'] && $form_state['values']['advagg_dir_htaccess']) {
    $conf['advagg_gzip_compression'] = $form_state['values']['advagg_gzip_compression'];
    $conf['advagg_dir_htaccess'] = $form_state['values']['advagg_dir_htaccess'];
    advagg_admin_recreate_htaccess();
  }
  if ($form_state['values']['advagg_dir_htaccess'] == FALSE) {
    $conf['advagg_dir_htaccess'] = FALSE;
    advagg_clearstatcache(TRUE, $css_path);
    advagg_clearstatcache(TRUE, $js_path);
    if (file_exists($css_path) || file_exists($js_path)) {
      file_unmanaged_delete($css_path);
      file_unmanaged_delete($js_path);
      drupal_set_message(t('Advanced CSS/JS Aggregation directory level htaccess files have been removed.'));
    }
  }
  elseif ($conf['advagg_dir_htaccess'] != $form_state['values']['advagg_dir_htaccess']) {
    $conf['advagg_dir_htaccess'] = $form_state['values']['advagg_dir_htaccess'];
    advagg_admin_recreate_htaccess();
  }
  advagg_clearstatcache(TRUE, $css_path);
  advagg_clearstatcache(TRUE, $js_path);
  if ($conf['advagg_dir_htaccess'] && (!file_exists($css_path) || !file_exists($js_path))) {
    advagg_admin_recreate_htaccess();
  }

  // If checksum mode changed, smart flush cache with new checksum mode selected.
  if ($conf['advagg_checksum_mode'] != $form_state['values']['advagg_checksum_mode']) {
    $conf['advagg_checksum_mode'] = $form_state['values']['advagg_checksum_mode'];
    advagg_admin_flush_cache();
  }

  // If advagg is enabled/disabled, smart flush the cache.
  if ($conf['advagg_enabled'] != $form_state['values']['advagg_enabled']) {
    advagg_admin_flush_cache();
  }
}

/**
 * Master reset button.
 */
function advagg_admin_master_reset() {
  cache_clear_all('*', 'cache_advagg', TRUE);
  cache_clear_all('*', 'cache_advagg_files_data', TRUE);
  cache_clear_all('*', 'cache_advagg_bundle_reuse', TRUE);
  db_query("TRUNCATE TABLE {advagg_files}");
  db_query("TRUNCATE TABLE {advagg_bundles}");

  module_invoke_all('advagg_master_reset');

  list($css_path, $js_path) = advagg_get_root_files_dir();
  file_scan_directory($css_path, '/.*/', array('callback' => 'file_unmanaged_delete'));
  file_scan_directory($js_path, '/.*/', array('callback' => 'file_unmanaged_delete'));

  drupal_set_message(t('Advanced CSS/JS Aggregation has been reset.'));
}

/**
 * Set up batch for first and last name loading.
 *
 * This is where Drupal's Batch API comes into play.
 * It's as simple as defining $batch, and then calling batch_set($batch)
 */
function advagg_admin_batch_rebuild($form, &$form_state) {
  $batch = array();

  // Set up the type of batch operation we will do.
  if ($form_state['clicked_button']['#post']['op'] == t('Force all counters to be increment by one')) {
    $increment = TRUE;
    $batch['title'] = t('Increment and rebuild aggregated files');

  }
  else {
    $increment = FALSE;
    $batch['title'] = t('Rebuilding aggregated files');
  }

  // Build batch operational data.
  $batch += array(
    'operations' => array(
      array('advagg_admin_rebuild_bundles', array($increment)),
    ),
    'finished' => 'advagg_admin_rebuild_bundles_done',
    'init_message' => t('Initializing...'),
    'progress_message' => t('@current of @total batch operations done.'),
    'error_message' => t('Rebuilding aggregated files encountered an error.'),
    'file' => drupal_get_path('module', 'advagg') . '/advagg.admin.inc',
  );

  // Run it
  batch_set($batch);
  batch_process('admin/config/advagg');
}

/**
 * Rebuild bundles.
 */
function advagg_admin_rebuild_bundles($increment, &$context) {
  // init batch.
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = db_query("SELECT COUNT(*) FROM (SELECT bundle_md5 FROM {advagg_bundles} GROUP BY bundle_md5) as temp")->fetchField();

    // Increment All Counters.
    if ($increment) {
      // TODO Please review the conversion of this statement to the D7 database API syntax.
      /* db_query("UPDATE {advagg_bundles} SET counter = counter + 1") */
      db_update('advagg_bundles')
  ->fields(array(
        'counter' => counter + 1,
      ))
  ->execute();
    }
  }

  // Set limit.
  $limit = 1;

  // Get list of all bundles.
  $result = db_query_range("SELECT bundle_md5 FROM {advagg_bundles} GROUP BY bundle_md5");
  while ($bundle_md5 = $result->fetchField()) {
    $filenames = advagg_rebuild_bundle($bundle_md5, '', TRUE);
    $filename = array();
    foreach ($filenames as $name => $data) {
      $filename[] = $name;
    }

    // Update our progress information.
    $context['sandbox']['progress']++;
    $context['message'] = t('%md5 Created', array('%md5' => $bundle_md5));
    $context['results'][] = $filename;
  }

  // Inform the batch engine that we are not finished,
  // and provide an estimation of the completion level we reached.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function advagg_admin_rebuild_bundles_done($success, $results, $operations) {
  if ($success) {
    // Here we do something meaningful with the results.
    $message = count($results) . ' files generated.';
    $message .= theme('item_list', array('items' => $results));
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array('%error_operation' => $error_operation[0], '@arguments' => print_r($error_operation[1], TRUE)));
  }
  drupal_set_message($message);
}

/**
 * Rebuild htaccess files.
 */
function advagg_admin_recreate_htaccess() {
  list($css_path, $js_path) = advagg_get_root_files_dir();
  $css_path .= '/.htaccess';
  $js_path .= '/.htaccess';

  file_unmanaged_delete($css_path);
  advagg_htaccess_check_generate($css_path, TRUE);

  file_unmanaged_delete($js_path);
  advagg_htaccess_check_generate($js_path, TRUE);

  drupal_set_message(t('Advanced CSS/JS Aggregation htaccess files rebuilt.'));
}

/**
 * Set or remove the AdvAggDisabled cookie.
 */
function advagg_admin_toggle_bypass_cookie() {
  global $base_path;
  $key = md5(drupal_get_private_key());
  // If the cookie does exist then remove it.
  if (advagg_cookie_bypass()) {
    setcookie(ADVAGG_COOKIE_NAME, '', -1, $base_path, '.' . $_SERVER['HTTP_HOST']);
    unset($_COOKIE[ADVAGG_COOKIE_NAME]);
    drupal_set_message(t('AdvAgg Bypass Cookie Removed.'));
  }
  // If the cookie does not exist then set it.
  else {
    setcookie(ADVAGG_COOKIE_NAME, $key, 0, $base_path, '.' . $_SERVER['HTTP_HOST']);
    $_COOKIE[ADVAGG_COOKIE_NAME] = $key;
    drupal_set_message(t('AdvAgg Bypass Cookie Set.'));
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function advagg_form_print_r($data) {
  $data = print_r($data, TRUE);
  $data = explode("\n", $data);
  $new_data = '';
  foreach ($data as $text) {
    $new_text = str_replace('    ', '', $text);
    if ($new_text == "(") {
      $new_data .= ' ' . $new_text;
    }
    else {
      $new_data .= "\n" . $text;
    }
  }
  $new_data = explode("\n", $new_data);
  $new_data = array_filter($new_data);
  $rows = count($new_data);
  $new_data = implode("\n", $new_data);
  return array($new_data, $rows);
}
