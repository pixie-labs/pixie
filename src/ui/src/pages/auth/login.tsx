import * as React from 'react';
import { AuthBox } from 'components/auth/auth-box';
import { BasePage } from './base';
import { auth0LoginRequest } from './utils';

export const LoginPage = () => (
  <BasePage>
    <AuthBox
      variant='login'
      toggleURL={`/auth/signup${window.location.search}`}
      onPrimaryButtonClick={auth0LoginRequest}
    />
  </BasePage>
);