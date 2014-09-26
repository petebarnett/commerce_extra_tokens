<?php

/**
 * @file
 * Defines additional tokens for order email.
 */

/**
 * Implements hook_token_info_alter().
 */
function commerce_extra_tokens_token_info_alter(&$data) {
  $data['tokens']['commerce-order']['commerce_extra_tokens_line_items'] = array(
    'name' => t('Commerce extra tokens line items'),
    'description' => t('A table containing line items of the commerce order.'),
  );
  $data['tokens']['commerce-order']['commerce_extra_tokens_total'] = array(
    'name' => t('Commerce extra tokens order total'),
    'description' => t('A table containing the totals and taxes of the commerce order.'),
  );
  $data['tokens']['commerce-order']['commerce_extra_tokens_payments'] = array(
    'name' => t('Commerce extra tokens payments'),
    'description' => t('A table containing the payment transactions on the order.'),
  );

  $data['tokens']['commerce-order']['product-ids'] = array(
    'name' => t('Commerce Extra Tokens: Product IDs on the order'),
    'description' => t('A comma separated list of product ids on the order')
  );

  // Find all term reference fields on product bundles
  $field_info = field_info_field_map();
  $product_term_ref_fields = array();
  foreach ($field_info as $field_name => $field) {
    if ($field['type'] == 'taxonomy_term_reference' && !empty($field['bundles']['commerce_product'])) {
      $product_term_ref_fields[] = $field_name;
      $data['tokens']['commerce-order']['product-terms-ids-' . $field_name] = array(
        'name' => t('Commerce Extra Tokens: Product term IDs from field !field_name', array('!field_name' => $field_name)),
        'description' => t('The unique term IDs referenced in on product line items through field !field_name', array('!field_name' => $field_name)),
      );
    }
  }

  // User Tokens
  $data['tokens']['user']['last-order'] = array(
    'name' => t('Commerce Extra Tokens: last order'),
    'description' => ('The last order for the user'),
    'type' => 'commerce-order'
  );

  $data['tokens']['user']['last-shipped-order'] = array(
    'name' => t('Commerce Extra Tokens: last shipped order'),
    'description' => ('The last shipped order for the user'),
    'type' => 'commerce-order'
  );

  $data['tokens']['user']['total-order-value-since-year-ago'] = array(
    'name' => t('Commerce Extra Tokens: total value of all orders completed in the past year'),
    'description' => ('The total value of all orders completed in the past year'),
  );

  $data['tokens']['user']['total-order-value-since-month-ago'] = array(
    'name' => t('Commerce Extra Tokens: total value of all orders completed in the past month'),
    'description' => ('The total value of all orders completed in the past month'),
  );

  $data['tokens']['user']['total-order-value'] = array(
    'name' => t('Commerce Extra Tokens: total value of all orders completed'),
    'description' => ('The total value of all orders ever completed'),
  );

  $data['tokens']['user']['last-abandonned-order'] = array(
    'name' => t('Commerce Extra Tokens: last abandonned order'),
    'description' => ('The last abandonned order for the user'),
    'type' => 'commerce-order'
  );
  $data['tokens']['user']['all-product-ids'] = array(
    'name' => t('Commerce Extra Tokens: all unique product ids'),
    'description' => ('Comma separated product ids of all products ever ordered'),
    'type' => 'commerce-order'
  );
}

/**
 * Implements hook_tokens().
 */
function commerce_extra_tokens_tokens($type, $tokens, array $data = array(), array $options = array()) {
  $replacements = array();
  if ($type == 'commerce-order' && !empty($data['commerce-order'])) {

    $order = $data['commerce-order'];

    // Find all term reference fields on product bundles
    $field_info = field_info_field_map();

    $product_term_reference_tokens = array();
    foreach ($field_info as $field_name => $field) {
      if ($field['type'] == 'taxonomy_term_reference' && !empty($field['bundles']['commerce_product'])) {
        $product_term_reference_tokens['product-terms-ids-' . $field_name] = TRUE;
      }
    }

    foreach ($tokens as $name => $original) {

      // A product term token
      if (!empty($product_term_reference_tokens[$name])) {
        $term_ref_field = substr($name, 18);
        $replacements[$original] = commerce_extra_tokens_unique_product_term_ids($data['commerce-order'], $term_ref_field);
        continue;        
      }

      switch ($name) {
        case 'commerce_extra_tokens_line_items':
          $order_items = commerce_extra_tokens_line_items($data['commerce-order']);
          $replacements[$original] = $order_items;
          break;
        case 'commerce_extra_tokens_total':
          $order_items = commerce_extra_tokens_total($data['commerce-order']);
          $replacements[$original] = $order_items;
          break;
        case 'commerce_extra_tokens_payments':
          $order_items = commerce_extra_tokens_payments($data['commerce-order']);
          $replacements[$original] = $order_items;
          break;
        case 'product-ids':
          $replacements[$original] = commerce_extra_tokens_get_product_ids_for_order($order);
          break;
      }
    }
  }

  if ($type == 'user' && !empty($data['user'])) {

    $user = $data['user'];

    $totals = NULL;

    foreach ($tokens as $name => $original) {
      switch ($name) {
        case 'last-order':
          $order = commerce_extra_tokens_last_order_by_user($user->uid);
          $replacements[$original] = empty($order->order_id) ? NULL : $order->order_id;
          break;
        case 'last-shipped-order':
          $order = commerce_extra_tokens_last_shipped_order_by_user($user->uid);
          $replacements[$original] = empty($order->order_id) ? NULL : $order->order_id;
          break;
        case 'last-abandonned-order':
          $order = commerce_extra_tokens_last_abandonned_order_by_user($user->uid);
          $replacements[$original] = empty($order->order_id) ? NULL : $order->order_id;
          break;
        case 'total-order-value-since-year-ago':
          if (empty($totals)) {
            $totals = commerce_extra_tokens_sum_order_values_for_intervals($user->uid);
          }
          $replacements[$original] = $totals['year'];
          break;
        case 'total-order-value-since-month-ago':
          if (empty($totals)) {
            $totals = commerce_extra_tokens_sum_order_values_for_intervals($user->uid);
          }
          $replacements[$original] = $totals['month'];
          break;
        case 'total-order-value':
          if (empty($totals)) {
            $totals = commerce_extra_tokens_sum_order_values_for_intervals($user->uid);
          }
          $replacements[$original] = $totals['ever'];
          break;
        case 'all-product-ids':
          $replacements[$original] = commerce_extra_tokens_get_product_ids_for_all_orders($user->uid);
          break;
      }
    }

    if ($order_tokens = token_find_with_prefix($tokens, 'last-order')) {
      $order = commerce_extra_tokens_last_order_by_user($user->uid);
      $replacements += token_generate('commerce-order', $order_tokens, array('commerce-order' => $order), $options);
    }

    if ($order_tokens = token_find_with_prefix($tokens, 'last-shipped-order')) {
      $order = commerce_extra_tokens_last_shipped_order_by_user($user->uid);
      $replacements += token_generate('commerce-order', $order_tokens, array('commerce-order' => $order), $options);
    }

    if ($order_tokens = token_find_with_prefix($tokens, 'last-abandonned-order')) {
      $order = commerce_extra_tokens_last_abandonned_order_by_user($user->uid);
      $replacements += token_generate('commerce-order', $order_tokens, array('commerce-order' => $order), $options);
    }
  }

  return $replacements;
}
