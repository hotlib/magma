/**
 * Copyright 2004-present Facebook. All Rights Reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow
 * @format
 */

import type {MetricGraphConfig} from '../insights/Metrics';

import NetworkMetrics from '../insights/NetworkMetrics';
import React from 'react';

const chartConfigs: MetricGraphConfig[] = [
  {
    label: 'Authorization',
    basicQueryConfigs: [],
    customQueryConfigs: [
      {
        resolveQuery: _ => `sum(eap_auth) by (code)`,
      },
    ],
  },
  {
    label: 'Active Sessions',
    basicQueryConfigs: [],
    customQueryConfigs: [
      {
        resolveQuery: _ => `sum(active_sessions)`,
      },
    ],
  },
  {
    label: 'Traffic In',
    basicQueryConfigs: [],
    customQueryConfigs: [
      {
        resolveQuery: _ => `sum(octets_in)`,
      },
    ],
  },
  {
    label: 'Traffic Out',
    basicQueryConfigs: [],
    customQueryConfigs: [
      {
        resolveQuery: _ => `sum(octets_out)`,
      },
    ],
  },
  {
    label: 'Throughput In',
    basicQueryConfigs: [],
    customQueryConfigs: [
      {
        resolveQuery: _ => `avg(rate(octets_in[5m]))`,
      },
    ],
  },
  {
    label: 'Throughput Out',
    basicQueryConfigs: [],
    customQueryConfigs: [
      {
        resolveQuery: _ => `avg(rate(octets_out[5m]))`,
      },
    ],
  },
  {
    label: 'Accounting Stops',
    basicQueryConfigs: [],
    customQueryConfigs: [
      {
        resolveQuery: _ => `sum(accounting_stop)`,
      },
    ],
  },
  {
    label: 'Session Create Latency',
    basicQueryConfigs: [],
    customQueryConfigs: [
      {
        resolveQuery: _ => `avg(create_session_lat)`,
      },
    ],
  },
  {
    label: 'Session Terminate',
    basicQueryConfigs: [],
    customQueryConfigs: [
      {
        resolveQuery: _ => `sum(session_terminate)`,
      },
    ],
  },
];

export default function() {
  return <NetworkMetrics configs={chartConfigs} />;
}
