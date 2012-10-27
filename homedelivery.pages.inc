<?php

/**
 * Menu callback; Show list of homedelivery types we can add.
 */
function homedelivery_add_page() {
  $item = menu_get_item();
  $links = system_admin_menu_block($item);

  foreach ($links as $link) {
    $items[] = l($link['title'], $link['href'], $item['localized_options'])
      . ': ' . filter_xss_admin($link['description']);
  }

  return theme('item_list', array('items' => $items));
}

/**
 * Present an homedelivery submission form.
 */
function homedelivery_add($type) {
  global $user;

  $types = homedelivery_types();
  $type = isset($type) ? str_replace('-', '_', $type) : NULL;
  if (empty($types[$type])) {
    return MENU_NOT_FOUND;
  }

  $homedelivery = entity_get_controller('homedelivery')->create($type);

  drupal_set_title(t('Create @name Order', array('@name' => $types[$type]->name)), PASS_THROUGH);
  return drupal_get_form($type . '_homedelivery_form', $homedelivery);
}

/**
 * Menu callback; presents the homedelivery editing form, or redirects to delete confirmation.
 *
 * @param $homedelivery
 *   The homedelivery object to edit.
 */
function homedelivery_page_edit($homedelivery) {
  $types = homedelivery_types();
  drupal_set_title(t('Edit <em>@type</em> Order made in @title', array('@type' => $types[$homedelivery->type]->name, '@title' => $homedelivery->title)), PASS_THROUGH);

  return drupal_get_form($homedelivery->type . '_homedelivery_form', $homedelivery);
}

/**
 * Form builder; Displays the homedelivery add/edit form.
 *
 * @param $form
 * @param $form_state
 * @param $homedelivery
 *   The homedelivery object to edit, which may be brand new.
 */
function homedelivery_form($form, &$form_state, $homedelivery) {

  // Set the id and identify this as an homedelivery edit form.
  $form['#id'] = 'homedelivery-form';

  // Save the homedelivery for later, in case we need it.
  $form['#homedelivery'] = $homedelivery;
  $form_state['homedelivery'] = $homedelivery;

  // Common fields. We don't have many.
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Record Date'),
    '#default_value' => $homedelivery->title,
    '#weight' => -5,
    '#required' => TRUE,
  );

	/*
  $form['revision'] = array(
    '#access' => user_access('administer homedeliverys'),
    '#type' => 'checkbox',
    '#title' => t('Create new revision'),
    '#default_value' => 0,
  );
	*/

  // Add the buttons.
  $form['buttons'] = array();
  $form['buttons']['#weight'] = 100;
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 5,
    '#submit' => array('homedelivery_form_submit'),
  );
  if (!empty($homedelivery->aid)) {
    $form['buttons']['delete'] = array(
      '#access' => user_access('delete homedeliverys'),
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 15,
      '#submit' => array('homedelivery_form_delete_submit'),
    );
  }

  $form['#validate'][] = 'homedelivery_form_validate';

  field_attach_form('homedelivery', $homedelivery, $form, $form_state);

  return $form;
}

function homedelivery_form_validate($form, &$form_state) {
  $homedelivery = $form_state['homedelivery'];

  // Field validation.
  field_attach_form_validate('homedelivery', $homedelivery, $form, $form_state);
}


function homedelivery_form_submit($form, &$form_state) {
  global $user;

  $homedelivery = &$form_state['homedelivery'];

  // Set the homedelivery's uid if it's being created at this time.
  if (empty($homedelivery->uid)) {
    $homedelivery->uid = $user->uid;
  }

  $homedelivery->title = $form_state['values']['title'];
  //$homedelivery->revision = $form_state['values']['revision'];

  // Notify field widgets.
  field_attach_submit('homedelivery', $homedelivery, $form, $form_state);

  // Save the homedelivery.
  homedelivery_save($homedelivery);

  // Notify the user.
  drupal_set_message(t('Home Delivery saved.'));

  $form_state['redirect'] = 'homedelivery/' . $homedelivery->aid;
}


function homedelivery_form_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $homedelivery = $form['#homedelivery'];
  $form_state['redirect'] = array('homedelivery/' . $homedelivery->aid . '/delete', array('query' => $destination));
}

/**
 * Displays an homedelivery.
 *
 * @param $homedelivery
 *   The homedelivery object to display.
 * @param $view_mode
 *   The view mode we want to display.
 */
function homedelivery_page_view($homedelivery, $view_mode = 'full') {
  // Remove previously built content, if exists.
  $homedelivery->content = array();

  if ($view_mode == 'teaser') {
    $homedelivery->content['title'] = array(
      '#markup' => filter_xss($homedelivery->title),
      '#weight' => -5,
    );
  }

  // Build fields content.
  field_attach_prepare_view('homedelivery', array($homedelivery->aid => $homedelivery), $view_mode);
  entity_prepare_view('homedelivery', array($homedelivery->aid => $homedelivery));
  $homedelivery->content += field_attach_view('homedelivery', $homedelivery, $view_mode);
  
  return $homedelivery->content;
}

/**
 * Form bulder; Asks for confirmation of homedelivery deletion.
 */
function homedelivery_delete_confirm($form, &$form_state, $homedelivery) {
  $form['#homedelivery'] = $homedelivery;
  // Always provide entity id in the same form key as in the entity edit form.
  $form['aid'] = array('#type' => 'value', '#value' => $homedelivery->aid);
  return confirm_form($form,
    t('Are you sure you want to delete this order made in %title?', array('%title' => $homedelivery->title)),
    'homedelivery/' . $homedelivery->aid,
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Executes homedelivery deletion.
 */
function homedelivery_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $homedelivery = homedelivery_load($form_state['values']['aid']);
    homedelivery_delete($form_state['values']['aid']);
    watchdog('homedelivery', '@type: deleted %title.', array('@type' => $homedelivery->type, '%title' => $homedelivery->title));

    $types = homedelivery_types();
    drupal_set_message(t('@type %title has been deleted.', array('@type' => $types[$homedelivery->type]->name, '%title' => $homedelivery->title)));
  }

  $form_state['redirect'] = '<front>';
}

/**
 * Menu callback; Displays a listing of recent homedeliverys.
 *
 * This doesn't really work yet because our presentation code doesn't show
 * the title.
 */
function homedelivery_page_list_recent() {
	global $user;

	//Added part for render header rows in a table.
	$header_homedelivery = array(
		array('data' => t('Type')),
		array('data' => t('Record Date')),
		//array('data' => t('Record Time')),
		array('data' => t('Customer Name')),
		array('data' => t('Customer Address')),
		array('data' => t('Phone Number')),
		array('data' => t('Full Order Info')),
		array('data' => t('Record Person')),
	);

  //$content = array();

  $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', 'homedelivery')
    ->propertyOrderBy('title', 'ASC')
    //->fieldCondition('field_cname', 'value', '', 'CONTAINS', 0)
    ->range(0, 5);
  $result = $query->execute();

  $homedeliverys = homedelivery_load_multiple(array_keys($result['homedelivery']));
  foreach ($homedeliverys as $homedelivery) {
    //The following line is not needed anymore since we will present whole data as a table. 
    //$content[$homedelivery->aid] = homedelivery_page_view($homedelivery, 'teaser');
    //instead we will define all values to be presented as $rows_homedelivery[].
    $rows_homedelivery [] = array(
    	array('data' => $homedelivery->type),
    	array('data' => $homedelivery->title),
    	array('data' => $homedelivery->field_cname['und'][0]['value']),
    	array('data' => $homedelivery->field_caddress['und'][0]['value']),
    	array('data' => $homedelivery->field_cnumber['und'][0]['value']),
    	array('data' => l(t('View'), 'homedelivery/' . $homedelivery -> aid)),
    	array('data' => l($user->name, 'user/' . $user -> uid)),
    );
  }
  //$caption_homedelivery = t('Table for home deliveries');
  //And instead of return $content
  //return $content;
  //We will implement theme() as return value.
  $build['homedelivery/recent'] =	array(
  	'#theme' => 'table',
		'#header' => $header_homedelivery,
		'#rows' => $rows_homedelivery,
		//'caption' => $caption_homedelivery
  );
  
  return $build;
}
