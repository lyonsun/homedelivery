<?php

/**
 * Controller for loading, creating, and saving homedeliverys.
 *
 * The default loader, which we extend, handles load() already. We only
 * need to add saving and creating.
 */
class HomedeliveryController extends DrupalDefaultEntityController {

  public function save($homedelivery) {
    $transaction = db_transaction();

    try {
      global $user;

      // Determine if we will be inserting a new homedelivery.
      $homedelivery->is_new = empty($homedelivery->aid);

      // Set the timestamp fields.
      if (empty($homedelivery->created)) {
        $homedelivery->created = REQUEST_TIME;
      }
      $homedelivery->changed = REQUEST_TIME;

      $homedelivery->revision_timestamp = REQUEST_TIME;
      $update_homedelivery = TRUE;

      // Give modules the opportunity to prepare field data for saving.
      field_attach_presave('homedelivery', $homedelivery);

      // When saving a new homedelivery revision, unset any existing $homedelivery->vid
      // to ensure a new revision will actually be created and store the old
      // revision ID in a separate property for homedelivery hook implementations.
      if (!$homedelivery->is_new && !empty($homedelivery->revision) && $homedelivery->vid) {
        $homedelivery->old_vid = $homedelivery->vid;
        unset($homedelivery->vid);
      }

      // If this is a new homedelivery...
      if ($homedelivery->is_new) {
        // Save the new homedelivery.
        drupal_write_record('homedelivery', $homedelivery);

        // Save the initial revision.
        $this->saveRevision($homedelivery, $user->uid);

        $op = 'insert';
      }
      else {
        // Save the updated homedelivery.
        drupal_write_record('homedelivery', $homedelivery, 'aid');

        // If a new homedelivery revision was requested, save a new record for that;
        // otherwise, update the homedelivery revision record that matches the value
        // of $homedelivery->vid.
        if (!empty($homedelivery->revision)) {
          $this->saveRevision($homedelivery, $user->uid);
        }
        else {
          $this->saveRevision($homedelivery, $user->uid, TRUE);
          $update_homedelivery = FALSE;
        }

        $op = 'update';
      }

      // If the revision ID is new or updated, save it to the homedelivery.
      if ($update_homedelivery) {
        db_update('homedelivery')
          ->fields(array('vid' => $homedelivery->vid))
          ->condition('aid', $homedelivery->aid)
          ->execute();
      }

      // Save fields.
      $function = 'field_attach_' . $op;
      $function('homedelivery', $homedelivery);

      module_invoke_all('entity_' . $op, $homedelivery, 'homedelivery');

      // Clear internal properties.
      unset($homedelivery->is_new);

      // Ignore slave server temporarily to give time for the saved order to be
      // propagated to the slave.
      db_ignore_slave();

      return $homedelivery;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('homedelivery', $e, NULL, WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Saves an homedelivery revision with the uid of the current user.
   *
   * @param $homedelivery
   *   The fully loaded homedelivery object.
   * @param $uid
   *   The user's uid for the current revision.
   * @param $update
   *   TRUE or FALSE indicating whether or not the existing revision should be
   *     updated instead of a new one created.
   */
  function saveRevision($homedelivery, $uid, $update = FALSE) {
    // Hold on to the homedelivery's original creator_uid but swap in the revision's
    // creator_uid for the momentary write.
    $temp_uid = $homedelivery->uid;
    $homedelivery->uid = $uid;

    // Update the existing revision if specified.
    if ($update) {
      drupal_write_record('homedelivery_revision', $homedelivery, 'vid');
    }
    else {
      // Otherwise insert a new revision. This will automatically update $homedelivery
      // to include the vid.
      drupal_write_record('homedelivery_revision', $homedelivery);
    }

    // Reset the order's creator_uid to the original value.
    $homedelivery->uid = $temp_uid;
 }

  /**
   * Deletes multiple homedeliverys by ID.
   *
   * @param $aids
   *   An array of homedelivery IDs to delete.
   * @return
   *   TRUE on success, FALSE otherwise.
   */
  public function delete($aids) {
    if (!empty($aids)) {
      $homedeliverys = $this->load($aids, array());

      $transaction = db_transaction();

      try {
        db_delete('homedelivery')
          ->condition('aid', $aids, 'IN')
          ->execute();

        db_delete('homedelivery_revision')
          ->condition('aid', $aids, 'IN')
          ->execute();

        foreach ($homedeliverys as $homedelivery_id => $homedelivery) {
          field_attach_delete('homedelivery', $homedelivery);
        }

        // Ignore slave server temporarily to give time for the
        // saved homedelivery to be propagated to the slave.
        db_ignore_slave();
      }
      catch (Exception $e) {
        $transaction->rollback();
        watchdog_exception('homedelivery', $e, NULL, WATCHDOG_ERROR);
        return FALSE;
      }

      module_invoke_all('entity_delete', $homedelivery, 'homedelivery');

      // Clear the page and block and homedelivery caches.
      cache_clear_all();
      $this->resetCache();
    }

    return TRUE;
  }

  /**
   * Create a default homedelivery.
   *
   * @param $type
   *   The machine-readable type of the homedelivery.
   *
   * @return
   *   An homedelivery object with all default fields initialized.
   */
  public function create($type = '') {
    return (object) array(
      'aid' => '',
      'type' => $type,
      'title' => '',
    );
  }
}
