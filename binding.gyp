{
  'targets': [
    {
      'target_name': 'repo',
      'include_dirs': [
        './deps/', './include/'
      ],
      'sources': ['deps/*.c', 'src/*.c', 'bindings.cc' ]
    }
  ]
}