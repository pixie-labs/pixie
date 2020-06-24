module.exports = {
  globals: {
    window: true,
  },
  setupFiles: [
    '<rootDir>/src/testing/enzyme-setup.js',
  ],
  setupFilesAfterEnv: [
    '<rootDir>/src/testing/jest-test-setup.js',
  ],
  moduleFileExtensions: [
    'js',
    'json',
    'jsx',
    'mjs',
    'ts',
    'tsx',
  ],
  moduleDirectories: [
    'node_modules',
    '<rootDir>/src',
  ],
  moduleNameMapper: {
    '^.+.(jpg|jpeg|png|gif|svg)$': '<rootDir>/src/testing/file-mock.js',
    '(\\.css|\\.scss$)|(normalize.css/normalize)|(^exports-loader)': 'identity-obj-proxy',
  },
  resolver: null,
  transform: {
    '^.+\\.jsx?$': 'babel-jest',
    '^.+\\.tsx?$': 'ts-jest',
    '^.+\\.toml$': 'jest-raw-loader',
  },
  testRegex: '.*-test\\.(ts|tsx|js|jsx)$',
  reporters: [
    'default',
    'jest-junit',
  ],
};