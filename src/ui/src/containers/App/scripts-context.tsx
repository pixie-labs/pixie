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

import * as React from 'react';
import { GetPxScripts, Script } from 'utils/script-bundle';
import { useUserInfo } from '@pixie-labs/api-react';

export interface ScriptsContextProps {
  scripts: Map<string, Script>;
  loading: boolean;
}

export const ScriptsContext = React.createContext<ScriptsContextProps>(null);

export const SCRATCH_SCRIPT: Script = {
  id: 'Scratch Pad',
  title: 'Scratch Pad',
  description: 'A clean slate for one-off scripts.\n'
    + 'This is ephemeral; it disappears upon changing scripts.',
  vis: '',
  code: 'import px\n\n'
    + '# Use this scratch pad to write and run one-off scripts.\n'
    + '# If you switch to another script, refresh, or close this browser tab, this script will disappear.\n\n',
  hidden: false,
};

export const ScriptsContextProvider = (props) => {
  const [user, loading, error] = useUserInfo();
  const [scripts, setScripts] = React.useState<Map<string, Script>>(new Map([['initial', {} as Script]]));

  // Lets us know if there is a pending setScripts. If there is, we're technically still loading for one more render.
  let nextScripts = scripts;

  React.useEffect(() => {
    if (!user || loading || error) return;
    GetPxScripts(user.orgID, user.orgName).then((list) => {
      const availableScripts = new Map<string, Script>(list.map((script) => [script.id, script]));
      availableScripts.set(SCRATCH_SCRIPT.id, SCRATCH_SCRIPT);
      setScripts(availableScripts);
      // eslint-disable-next-line react-hooks/exhaustive-deps
      nextScripts = availableScripts;
    });
    // Monitoring the user's ID and their org rather than the whole object, to avoid excess renders.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [user?.id, user?.orgID, user?.orgName, loading, error]);

  const reallyLoading = loading || nextScripts !== scripts;
  const context = React.useMemo(() => ({ scripts, loading: reallyLoading }), [scripts, reallyLoading]);

  return (
    <ScriptsContext.Provider value={context}>
      {props.children}
    </ScriptsContext.Provider>
  );
};
