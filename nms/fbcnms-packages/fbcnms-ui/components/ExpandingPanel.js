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
import ExpandMoreIcon from '@material-ui/icons/ExpandMore';
import ExpansionPanel from '@material-ui/core/ExpansionPanel';
import ExpansionPanelDetails from '@material-ui/core/ExpansionPanelDetails';
import ExpansionPanelSummary from '@material-ui/core/ExpansionPanelSummary';
import Text from './design-system/Text';
import classNames from 'classnames';
import {makeStyles} from '@material-ui/styles';

const useStyles = makeStyles(theme => ({
  expansionPanel: {
    padding: '24px 0px',
    borderRadius: '4px',
    '&:before': {
      content: 'none',
    },
    boxShadow: '0px 1px 4px 0px rgba(0,0,0,0.17)',
  },
  expansionPanelSummary: {
    '&&': {
      display: 'flex',
      flexDirection: 'row',
      padding: '0px 24px',
      minHeight: 'auto',
    },
  },
  expandIcon: {
    padding: '0px',
    marginRight: '0px',
  },
  summaryContent: {
    '&&': {
      alignItems: 'center',
      margin: 0,
      cursor: 'default',
    },
  },
  panelTitle: {
    fontSize: '20px',
    color: theme.palette.blueGrayDark,
    lineHeight: '28px',
    fontWeight: 500,
    flexGrow: 1,
  },
  panelDetails: {
    padding: '0px 24px',
    display: 'flex',
    flexDirection: 'column',
    marginTop: '16px',
  },
}));

type Props = {
  title: string,
  children: React.Node,
  className?: string,
  detailsPaneClass?: string,
  rightContent?: React.Node,
};

const ExpandingPanel = ({
  className,
  detailsPaneClass,
  children,
  title,
  rightContent,
}: Props) => {
  const classes = useStyles();
  const [isExpanded, setIsExpanded] = React.useState(true);
  return (
    <ExpansionPanel
      className={classNames(className, classes.expansionPanel)}
      defaultExpanded={true}
      expanded={isExpanded}>
      <ExpansionPanelSummary
        className={classes.expansionPanelSummary}
        classes={{
          expandIcon: classes.expandIcon,
          content: classes.summaryContent,
        }}
        expandIcon={<ExpandMoreIcon className={classes.expandButton} />}
        IconButtonProps={{onClick: () => setIsExpanded(!isExpanded)}}>
        <Text className={classes.panelTitle}>{title}</Text>
        {rightContent}
      </ExpansionPanelSummary>
      <ExpansionPanelDetails
        className={classNames(detailsPaneClass, classes.panelDetails)}>
        {children}
      </ExpansionPanelDetails>
    </ExpansionPanel>
  );
};

export default ExpandingPanel;
