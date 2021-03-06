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

import { SCRATCH_SCRIPT, ScriptsContext } from 'containers/App/scripts-context';
import * as React from 'react';
import urlParams from 'utils/url-params';
import { containsMutation } from '@pixie-labs/api';

import { ScriptContext } from 'context/script-context';
import { ResultsContext } from 'context/results-context';
import { argsForVis } from 'utils/args-utils';
import {
  LiveViewPage,
  LiveViewPageScriptIds,
  matchLiveViewEntity,
} from 'containers/live-widgets/utils/live-view-params';

type LoadScriptState = 'unloaded' | 'url-loaded' | 'url-skipped' | 'context-loaded';

export function ScriptLoader() {
  const [loadState, setLoadState] = React.useState<LoadScriptState>('unloaded');
  const { scripts, loading: loadingScripts } = React.useContext(ScriptsContext);
  const {
    pxl, vis, args, id, liveViewPage, setScript, execute, parseVisOrShowError, argsForVisOrShowError,
  } = React.useContext(ScriptContext);

  const { clearResults } = React.useContext(ResultsContext);
  const ref = React.useRef({
    urlLoaded: false,
    execute,
  });

  ref.current.execute = execute;

  // Execute the default scripts if script was not loaded from the URL.
  React.useEffect(() => {
    if (loadState === 'url-skipped') {
      if (pxl && vis) {
        execute({
          pxl,
          vis,
          args,
          id,
          liveViewPage,
        });
        setLoadState('context-loaded');
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [execute, loadState, pxl, vis]);

  React.useEffect(() => {
    const subscription = urlParams.onChange.subscribe((urlInfo) => {
      if (loadingScripts) return;

      const { pathname } = urlInfo;
      const urlArgs = urlInfo.args;
      const urlScriptId = urlInfo.scriptId;

      let entityPage;
      let entityParams;

      // default live view page should be px/cluster.
      if ((pathname === '/live' || pathname === '/') && !pxl) {
        entityPage = LiveViewPage.Cluster;
        entityParams = {};
      } else {
        const entity = matchLiveViewEntity(pathname);
        entityPage = entity.page;
        entityParams = entity.params;
      }

      const selectedId = entityPage === LiveViewPage.Default ? urlScriptId : LiveViewPageScriptIds.get(entityPage);

      if (!scripts.has(selectedId)) {
        setLoadState((state) => {
          if (state !== 'unloaded') {
            return state;
          }
          return 'url-skipped';
        });
        return;
      }

      const script = scripts.get(selectedId);
      const parsedVis = parseVisOrShowError(script.vis);
      const parsedArgs = argsForVis(parsedVis, { ...urlArgs, ...entityParams }, selectedId);
      if (!parsedVis && !parsedArgs) {
        return;
      }

      const execArgs = {
        liveViewPage: entityPage,
        pxl: script.code,
        vis: parsedVis,
        args: parsedArgs,
        id: selectedId,
        skipURLUpdate: true,
      };
      clearResults();
      setScript(execArgs.vis, execArgs.pxl, execArgs.args, execArgs.id, execArgs.liveViewPage);

      // Use this hack because otherwise args are not set when you first load a page.
      if (!argsForVisOrShowError(parsedVis, { ...urlArgs, ...entityParams }, selectedId)) {
        return;
      }
      // The Scratch Pad script starts with just comments and no code. Running that would be an error.
      if (script.id === SCRATCH_SCRIPT.id) return;

      if (!containsMutation(execArgs.pxl)) {
        ref.current.execute(execArgs);
      }
      setLoadState((state) => {
        if (state !== 'unloaded') {
          return state;
        }
        return 'url-loaded';
      });
    });
    return () => {
      subscription.unsubscribe();
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [scripts, loadingScripts]);
  return null;
}
