export interface LoggerEnv {
  // Use console for simplicity
  readonly logger: Pick<
    typeof console,
    'info' | 'warn' | 'error' | 'debug' | 'log'
  >;
}
