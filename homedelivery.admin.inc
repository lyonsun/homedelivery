<?php

/**
 * Menu callback; List all homedelivery types available.
 */
function homedelivery_overview_types() {
  foreach (homedelivery_types() as $type => $info) {
    $type_url_str = str_replace('_', '-', $type);
    $label = t('Manage @type Orders', array('@type' => $info->name));
    $items[] = l($label, 'admin/structure/homedeliverys/manage/' . $type_url_str);
  }

  return theme('item_list', array('items' => $items));
}

/**
 * Menu callback; Artwork information page.
 *
 * @param object $homedelivery_type
 */
function homedelivery_information($homedelivery_type) {
  return $homedelivery_type->name . ': ' . $homedelivery_type->description;
}
