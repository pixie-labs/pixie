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

import { DARK_THEME, SnackbarProvider, VersionInfo } from '@pixie-labs/components';
import Vizier from 'containers/App/vizier';
import PixieCookieBanner from 'common/cookie-banner';
import { LD_CLIENT_ID } from 'containers/constants';
import {
  Redirect, Route, Router, Switch,
} from 'react-router-dom';
import { isProd, PIXIE_CLOUD_VERSION } from 'utils/env';
import history from 'utils/pl-history';

import {
  createStyles, ThemeProvider, withStyles,
} from '@material-ui/core/styles';

import * as React from 'react';
import * as ReactDOM from 'react-dom';
import { CssBaseline } from '@material-ui/core';
import { withLDProvider } from 'launchdarkly-react-client-sdk';
import { AuthRouter } from 'pages/auth/auth';
import 'typeface-roboto';
import 'typeface-roboto-mono';
import { PixieAPIContextProvider, useIsAuthenticated } from '@pixie-labs/api-react';

// This side-effect-only import has to be a `require`, or else it gets erroneously optimized away during compilation.
require('./wdyr');

const RedirectWithArgs = (props) => {
  const {
    from,
    to,
    exact,
    location,
  } = props;

  return (
    <Redirect
      from={from}
      exact={exact}
      to={{ pathname: to, search: location.search }}
    />
  );
};

export const App: React.FC = () => {
  const { authenticated, loading } = useIsAuthenticated();

  const authRedirectUri = window.location.pathname.length > 1
    ? encodeURIComponent(window.location.pathname + window.location.search)
    : '';
  const authRedirectTo = authRedirectUri ? `/login?redirect_uri=${authRedirectUri}` : '/login';

  return loading ? null : (
    <>
      <SnackbarProvider>
        <Router history={history}>
          <div className='center-content'>
            <Switch>
              <Route path='/auth' component={AuthRouter} />
              <RedirectWithArgs exact from='/login' to='/auth/login' />
              <RedirectWithArgs exact from='/logout' to='/auth/logout' />
              <RedirectWithArgs exact from='/signup' to='/auth/signup' />
              <RedirectWithArgs exact from='/auth-complete' to='/auth/cli-auth-complete' />
              {
                // 404s are handled within the Vizier route, after the user authenticates.
                // Logged out users get redirected to /login before the possibility of a 404 is checked.
                authenticated ? <Route component={Vizier} />
                  : <Redirect from='/*' to={authRedirectTo} />
              }
            </Switch>
          </div>
        </Router>
        {!isProd() ? <VersionInfo cloudVersion={PIXIE_CLOUD_VERSION} /> : null}
      </SnackbarProvider>
      <PixieCookieBanner />
    </>
  );
};

// TODO(zasgar): Cleanup these styles. We probably don't need them all after we
// fix all of material styling.
const styles = () => createStyles({
  '@global': {
    '#root': {
      height: '100%',
      width: '100%',
    },
    html: {
      height: '100%',
    },
    body: {
      height: '100%',
      overflow: 'hidden',
      margin: 0,
      boxSizing: 'border-box',
    },
    ':focus': {
      outline: 'none !important',
    },
    // TODO(zasgar): remove this center content global.
    '.center-content': {
      height: '100%',
      width: '100%',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
    },
  },
});

let StyledApp = withStyles(styles)(App);

if (LD_CLIENT_ID !== '') {
  StyledApp = withLDProvider({
    clientSideID: LD_CLIENT_ID,
  })(StyledApp);
}

ReactDOM.render(
  <ThemeProvider theme={DARK_THEME}>
    <CssBaseline />
    <PixieAPIContextProvider apiKey=''>
      <StyledApp />
    </PixieAPIContextProvider>
  </ThemeProvider>, document.getElementById('root'));
