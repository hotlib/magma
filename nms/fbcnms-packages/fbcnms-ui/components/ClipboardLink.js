/**
 * Copyright 2004-present Facebook. All Rights Reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow
 * @format
 */

import * as React from 'react';
import Button from '@material-ui/core/Button';
import IconButton from '@material-ui/core/IconButton';
import Tooltip from '@material-ui/core/Tooltip';

import copy from 'copy-to-clipboard';
import {useState} from 'react';

type Props = {
  ...React.ElementConfig<typeof Tooltip>,
  children: (props: {copyString: (content: string) => void}) =>
    | React.Element<typeof Button>
    | React.Element<typeof IconButton>,
  // We set the title appropriately later
  title?: React.Node,
};

const COPIED_MESSAGE = 'Copied to clipboard!';

/* Wrap a button with this component to copy a string to clipboard when
 * the button is clicked. After the button is clicked, a tooltip will
 * pop up saying the copying was successful. Tooltip custom props can be
 * passed into this component directly.
 */
export default function ClipboardLink(props: Props) {
  if (props.title != null) {
    return <ClipboardLinkWithTitle {...props} />;
  }
  return <ClipboardLinkNoTitle {...props} />;
}

/* Since the logic and states are diffferent depending on whether a title for
 * the tooltip is passed in, we have 2 different components below for each
 * scenario.
 */

// If they pass in a title, we need to change that title briefly to
// COPIED_MESSAGE whenever the content is copied.
function ClipboardLinkWithTitle(props: Props) {
  const [currentTitle, setCurrentTitle] = useState(props.title);
  return (
    <Tooltip
      {...props}
      title={currentTitle}
      onClose={() => setCurrentTitle(props.title)}>
      {props.children({
        copyString: content => {
          copy(content);
          setCurrentTitle(COPIED_MESSAGE);
        },
      })}
    </Tooltip>
  );
}

// If they don't pass in a title, there should be no COPIED_MESSAGE tooltip
// shown until the content is copied.
function ClipboardLinkNoTitle(props: Props) {
  const [showTooltip, setShowTooltip] = useState(false);
  return (
    <Tooltip
      {...props}
      title={COPIED_MESSAGE}
      open={showTooltip}
      onClose={() => setShowTooltip(false)}>
      {props.children({
        copyString: content => {
          copy(content);
          setShowTooltip(true);
        },
      })}
    </Tooltip>
  );
}
