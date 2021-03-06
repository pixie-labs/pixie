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

import {
  CommandAutocomplete,
  TabStop, PixieCommandIcon, PixieCommandHint,
} from '@pixie-labs/components';
import { ScriptsContext } from 'containers/App/scripts-context';
import * as React from 'react';
import { ClusterContext } from 'common/cluster-context';

import {
  Button, createStyles, makeStyles, Theme, Tooltip,
} from '@material-ui/core';
import Card from '@material-ui/core/Card';
import Modal from '@material-ui/core/Modal';

import { ScriptContext, ExecuteArguments } from 'context/script-context';
import { containsMutation, GQLTabSuggestion } from '@pixie-labs/api';
import { entityPageForScriptId } from 'containers/live-widgets/utils/live-view-params';
import { useAutocomplete } from '@pixie-labs/api-react';
import { TabSuggestion } from '@pixie-labs/components/src';
import { ParseFormatStringToTabStops } from './autocomplete-parser';
import { entityTypeToString } from './autocomplete-utils';

interface NewCommandInputProps {
  open: boolean;
  onClose: () => void;
}

interface CurrentInput {
  text: string;
}

const useHintStyles = makeStyles((theme: Theme) => (createStyles({
  hotkeyHint: {
    color: theme.typography.h3.color,
    opacity: 0.6,
    flex: '0 0 auto',
    position: 'relative',
    bottom: theme.spacing(0.25), // Vertically aligns the shift/enter symbols with lowercase letters
    userSelect: 'none',
    paddingLeft: theme.spacing(1.25),

    '& svg': {
      pointerEvents: 'none',
      fill: 'transparent',
      stroke: 'currentColor',
      width: 'auto',

      '& > *': {
        strokeWidth: '15',
      },
    },
  },
})));

const PixieCommandSubmitHint: React.FC<{execute: () => void; disabled: boolean}> = ({ execute, disabled }) => {
  const classes = useHintStyles();
  const disabledHint = 'Enter a valid command, then click or press Shift+Enter';
  const activeHint = 'Click or press Shift+Enter to run';
  // The extra wrapper div is to prevent the tooltip from disabling itself when the button is disabled (MUI feature).
  return (
    <Tooltip title={disabled ? disabledHint : activeHint} enterDelay={200}>
      <div>
        <Button
          onClick={execute}
          className={classes.hotkeyHint}
          tabIndex={-1}
          disabled={disabled}
          aria-label={disabled ? disabledHint : activeHint}
        >
          <PixieCommandHint />
        </Button>
      </div>
    </Tooltip>
  );
};

const useStyles = makeStyles(() => (createStyles({
  card: {
    position: 'absolute',
    width: '760px',
    top: '40%',
    left: '50%',
    transform: 'translate(-50%, -20vh)',
  },
  input: {
    maxHeight: '60vh',
  },
})));

const CommandInput: React.FC<NewCommandInputProps> = ({ open, onClose }) => {
  const classes = useStyles();
  const [tabStops, setTabStops] = React.useState<Array<TabStop>>([]);
  const [tabSuggestions, setTabSuggestions] = React.useState<Array<GQLTabSuggestion>>([]);
  const [isValid, setIsValid] = React.useState(false);
  const { selectedClusterUID } = React.useContext(ClusterContext);

  const {
    execute, setScript, parseVisOrShowError, argsForVisOrShowError,
  } = React.useContext(ScriptContext);
  const { scripts } = React.useContext(ScriptsContext);
  const [currentInput] = React.useState({} as CurrentInput);

  const runAutocomplete = useAutocomplete(selectedClusterUID);
  const onChange = React.useCallback((input, cursor, action, updatedTabStops) => {
    if (updatedTabStops !== null) {
      setTabStops(updatedTabStops);
      setIsValid(false);
    }
    currentInput.text = input;

    return runAutocomplete(input, cursor, action).then((result) => {
      if (input !== currentInput.text) return;
      setIsValid(result.isExecutable);
      setTabStops(ParseFormatStringToTabStops(result.formattedInput));
      const completions = result.tabSuggestions.map((s) => {
        const suggestions = s.suggestions.map((suggestion, i) => ({
          type: 'item',
          id: suggestion.name + i,
          title: suggestion.name,
          itemType: entityTypeToString(suggestion.kind),
          description: suggestion.description,
          highlights: suggestion.matchedIndexes,
          state: suggestion.state,
        }));

        return {
          index: s.tabIndex,
          executableAfterSelect: s.executableAfterSelect,
          suggestions,
        };
      });
      setTabSuggestions(completions);
    });
    // selectedClusterUID is watched to avoid stale results
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [runAutocomplete, selectedClusterUID, currentInput]);

  // Make an API call to get a list of initial suggestions when the command input is first loaded.
  React.useEffect(() => {
    onChange('', 0, 'EDIT', null).then();
    // We only want this useEffect to be called the first time the command input is loaded.
    // eslint-disable-next-line
  }, []);

  const onSubmit = React.useCallback(() => {
    if (isValid) {
      const script = scripts.get(tabStops[0].Value);
      const vis = parseVisOrShowError(script.vis);
      const args = {};
      tabStops.forEach((ts, idx) => {
        if (idx !== 0) { // Skip the "script" argument.
          args[ts.Label] = ts.Value;
        }
      });
      const parsedArgs = argsForVisOrShowError(vis, args);

      if (script && vis && parsedArgs) {
        const execArgs: ExecuteArguments = {
          liveViewPage: entityPageForScriptId(script.id),
          pxl: script.code,
          vis,
          id: script.id,
          args: parsedArgs,
        };
        setTabStops([{ CursorPosition: 0, Index: 1, Value: '' }]);
        setIsValid(false);
        setScript(execArgs.vis, execArgs.pxl, execArgs.args, execArgs.id, execArgs.liveViewPage);
        if (!containsMutation(execArgs.pxl)) {
          execute(execArgs);
        }
        onClose();
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [tabStops, isValid, execute, onClose, scripts]);

  const convertedSuggestions: TabSuggestion[] = tabSuggestions
    .map((s) => ({ index: s.tabIndex, ...s } as TabSuggestion));

  return (
    <Modal open={open} onClose={onClose} BackdropProps={{}}>
      <Card className={classes.card}>
        <CommandAutocomplete
          className={classes.input}
          onSubmit={onSubmit}
          onChange={onChange}
          completions={convertedSuggestions}
          tabStops={tabStops}
          placeholder='Type a script or entity...'
          isValid={isValid}
          prefix={<PixieCommandIcon />}
          suffix={<PixieCommandSubmitHint execute={onSubmit} disabled={!isValid} />}
        />
      </Card>
    </Modal>
  );
};

export default CommandInput;
