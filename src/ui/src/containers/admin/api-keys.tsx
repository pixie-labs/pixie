/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

import Table from '@material-ui/core/Table';
import IconButton from '@material-ui/core/IconButton';
import Input from '@material-ui/core/Input';
import MenuItem from '@material-ui/core/MenuItem';
import TableBody from '@material-ui/core/TableBody';
import TableHead from '@material-ui/core/TableHead';
import TableRow from '@material-ui/core/TableRow';
import Actions from '@material-ui/icons/MoreHoriz';
import Copy from '@material-ui/icons/FileCopy';
import Delete from '@material-ui/icons/DeleteForever';
import Visibility from '@material-ui/icons/Visibility';
import VisibilityOff from '@material-ui/icons/VisibilityOff';
import { distanceInWords } from 'date-fns';
import * as React from 'react';
import { useAPIKeys } from '@pixie-labs/api-react';
import {
  AdminTooltip, StyledTableCell, StyledTableHeaderCell,
  StyledLeftTableCell, StyledRightTableCell,
} from './utils';
import {
  UseKeyListStyles, KeyListItemIcon, KeyListItemText, KeyListMenu,
} from './key-list';

interface APIKeyDisplay {
  id: string;
  idShort: string;
  createdAt: string;
  key: string;
  desc: string;
}

export function formatAPIKey(apiKey): APIKeyDisplay {
  const now = new Date();
  return {
    id: apiKey.id,
    idShort: apiKey.id.split('-').pop(),
    createdAt: `${distanceInWords(new Date(apiKey.createdAtMs), now, { addSuffix: false })} ago`,
    key: apiKey.key,
    desc: apiKey.desc,
  };
}

export const APIKeyRow = ({ apiKey }) => {
  const classes = UseKeyListStyles();
  const [showKey, setShowKey] = React.useState(false);

  const [open, setOpen] = React.useState<boolean>(false);
  const [anchorEl, setAnchorEl] = React.useState(null);

  const [{ deleteAPIKey }] = useAPIKeys();

  const openMenu = React.useCallback((event) => {
    setOpen(true);
    setAnchorEl(event.currentTarget);
  }, []);

  const closeMenu = React.useCallback(() => {
    setOpen(false);
    setAnchorEl(null);
  }, []);

  return (
    <TableRow key={apiKey.id}>
      <AdminTooltip title={apiKey.id}>
        <StyledLeftTableCell>{apiKey.idShort}</StyledLeftTableCell>
      </AdminTooltip>
      <StyledTableCell>{apiKey.createdAt}</StyledTableCell>
      <StyledTableCell>{apiKey.desc}</StyledTableCell>
      <StyledTableCell>
        <Input
          className={classes.keyValue}
          id='api-key'
          fullWidth
          readOnly
          disableUnderline
          type={showKey ? 'text' : 'password'}
          value={apiKey.key}
        />
      </StyledTableCell>
      <StyledRightTableCell>
        <IconButton
          size='small'
          classes={{ sizeSmall: classes.actionsButton }}
          onClick={openMenu}
        >
          <Actions />
        </IconButton>
        <KeyListMenu
          open={open}
          onClose={closeMenu}
          anchorEl={anchorEl}
          getContentAnchorEl={null}
          anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}
        >
          <MenuItem key='show' alignItems='center' onClick={() => setShowKey(!showKey)}>
            <KeyListItemIcon>
              {showKey ? <Visibility /> : <VisibilityOff />}
            </KeyListItemIcon>
            <KeyListItemText primary={showKey ? 'Hide value' : 'Show value'} />
          </MenuItem>
          <MenuItem
            key='copy'
            alignItems='center'
            onClick={() => navigator.clipboard.writeText(apiKey.key)}
          >
            <KeyListItemIcon className={classes.copyBtn}>
              <Copy />
            </KeyListItemIcon>
            <KeyListItemText primary='Copy value' />
          </MenuItem>
          <MenuItem
            key='delete'
            alignItems='center'
            onClick={() => deleteAPIKey(apiKey.id)}
          >
            <KeyListItemIcon className={classes.copyBtn}>
              <Delete />
            </KeyListItemIcon>
            <KeyListItemText primary='Delete' />
          </MenuItem>
        </KeyListMenu>
      </StyledRightTableCell>
    </TableRow>
  );
};

export const APIKeysTable = () => {
  const classes = UseKeyListStyles();
  const [{ apiKeys: apiKeysRaw }, loading, error] = useAPIKeys();
  if (loading) {
    return <div className={classes.error}>Loading...</div>;
  }
  if (error) {
    return <div className={classes.error}>{error.toString()}</div>;
  }

  const apiKeys = (apiKeysRaw ?? []).map((key) => formatAPIKey(key));
  return (
    <>
      <Table>
        <TableHead>
          <TableRow>
            <StyledTableHeaderCell>ID</StyledTableHeaderCell>
            <StyledTableHeaderCell>Created</StyledTableHeaderCell>
            <StyledTableHeaderCell>Description</StyledTableHeaderCell>
            <StyledTableHeaderCell>Value</StyledTableHeaderCell>
            <StyledTableHeaderCell>Actions</StyledTableHeaderCell>
          </TableRow>
        </TableHead>
        <TableBody>
          {apiKeys.map((apiKey: APIKeyDisplay) => (
            <APIKeyRow key={apiKey.id} apiKey={apiKey} />
          ))}
        </TableBody>
      </Table>
    </>
  );
};
